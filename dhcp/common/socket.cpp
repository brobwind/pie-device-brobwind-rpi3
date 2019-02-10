/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "socket.h"

#include "message.h"
#include "utils.h"

#include <errno.h>
#include <linux/if_packet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

// Combine the checksum of |buffer| with |size| bytes with |checksum|. This is
// used for checksum calculations for IP and UDP.
static uint32_t addChecksum(const uint8_t* buffer,
                            size_t size,
                            uint32_t checksum) {
    const uint16_t* data = reinterpret_cast<const uint16_t*>(buffer);
    while (size > 1) {
        checksum += *data++;
        size -= 2;
    }
    if (size > 0) {
        // Odd size, add the last byte
        checksum += *reinterpret_cast<const uint8_t*>(data);
    }
    // msw is the most significant word, the upper 16 bits of the checksum
    for (uint32_t msw = checksum >> 16; msw != 0; msw = checksum >> 16) {
        checksum = (checksum & 0xFFFF) + msw;
    }
    return checksum;
}

// Convenienct template function for checksum calculation
template<typename T>
static uint32_t addChecksum(const T& data, uint32_t checksum) {
    return addChecksum(reinterpret_cast<const uint8_t*>(&data), sizeof(T), checksum);
}

// Finalize the IP or UDP |checksum| by inverting and truncating it.
static uint32_t finishChecksum(uint32_t checksum) {
    return ~checksum & 0xFFFF;
}

Socket::Socket() : mSocketFd(-1) {
}

Socket::~Socket() {
    if (mSocketFd != -1) {
        ::close(mSocketFd);
        mSocketFd = -1;
    }
}


Result Socket::open(int domain, int type, int protocol) {
    if (mSocketFd != -1) {
        return Result::error("Socket already open");
    }
    mSocketFd = ::socket(domain, type, protocol);
    if (mSocketFd == -1) {
        return Result::error("Failed to open socket: %s", strerror(errno));
    }
    return Result::success();
}

Result Socket::bind(const void* sockaddr, size_t sockaddrLength) {
    if (mSocketFd == -1) {
        return Result::error("Socket not open");
    }

    int status = ::bind(mSocketFd,
                        reinterpret_cast<const struct sockaddr*>(sockaddr),
                        sockaddrLength);
    if (status != 0) {
        return Result::error("Unable to bind raw socket: %s", strerror(errno));
    }

    return Result::success();
}

Result Socket::bindIp(in_addr_t address, uint16_t port) {
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = address;

    return bind(&sockaddr, sizeof(sockaddr));
}

Result Socket::bindRaw(unsigned int interfaceIndex) {
    struct sockaddr_ll sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sll_family = AF_PACKET;
    sockaddr.sll_protocol = htons(ETH_P_IP);
    sockaddr.sll_ifindex = interfaceIndex;

    return bind(&sockaddr, sizeof(sockaddr));
}

Result Socket::sendOnInterface(unsigned int interfaceIndex,
                               in_addr_t destinationAddress,
                               uint16_t destinationPort,
                               const Message& message) {
    if (mSocketFd == -1) {
        return Result::error("Socket not open");
    }

    char controlData[CMSG_SPACE(sizeof(struct in_pktinfo))] = { 0 };
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(destinationPort);
    addr.sin_addr.s_addr = destinationAddress;

    struct msghdr header;
    memset(&header, 0, sizeof(header));
    struct iovec iov;
    // The struct member is non-const since it's used for receiving but it's
    // safe to cast away const for sending.
    iov.iov_base = const_cast<uint8_t*>(message.data());
    iov.iov_len = message.size();
    header.msg_name = &addr;
    header.msg_namelen = sizeof(addr);
    header.msg_iov = &iov;
    header.msg_iovlen = 1;
    header.msg_control = &controlData;
    header.msg_controllen = sizeof(controlData);

    struct cmsghdr* controlHeader = CMSG_FIRSTHDR(&header);
    controlHeader->cmsg_level = IPPROTO_IP;
    controlHeader->cmsg_type = IP_PKTINFO;
    controlHeader->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
    auto packetInfo =
        reinterpret_cast<struct in_pktinfo*>(CMSG_DATA(controlHeader));
    memset(packetInfo, 0, sizeof(*packetInfo));
    packetInfo->ipi_ifindex = interfaceIndex;

    ssize_t status = ::sendmsg(mSocketFd, &header, 0);
    if (status <= 0) {
        return Result::error("Failed to send packet: %s", strerror(errno));
    }
    return Result::success();
}

Result Socket::sendRawUdp(in_addr_t source,
                          uint16_t sourcePort,
                          in_addr_t destination,
                          uint16_t destinationPort,
                          unsigned int interfaceIndex,
                          const Message& message) {
    struct iphdr ip;
    struct udphdr udp;

    ip.version = IPVERSION;
    ip.ihl = sizeof(ip) >> 2;
    ip.tos = 0;
    ip.tot_len = htons(sizeof(ip) + sizeof(udp) + message.size());
    ip.id = 0;
    ip.frag_off = 0;
    ip.ttl = IPDEFTTL;
    ip.protocol = IPPROTO_UDP;
    ip.check = 0;
    ip.saddr = source;
    ip.daddr = destination;
    ip.check = finishChecksum(addChecksum(ip, 0));

    udp.source = htons(sourcePort);
    udp.dest = htons(destinationPort);
    udp.len = htons(sizeof(udp) + message.size());
    udp.check = 0;

    uint32_t udpChecksum = 0;
    udpChecksum = addChecksum(ip.saddr, udpChecksum);
    udpChecksum = addChecksum(ip.daddr, udpChecksum);
    udpChecksum = addChecksum(htons(IPPROTO_UDP), udpChecksum);
    udpChecksum = addChecksum(udp.len, udpChecksum);
    udpChecksum = addChecksum(udp, udpChecksum);
    udpChecksum = addChecksum(message.data(), message.size(), udpChecksum);
    udp.check = finishChecksum(udpChecksum);

    struct iovec iov[3];

    iov[0].iov_base = static_cast<void*>(&ip);
    iov[0].iov_len = sizeof(ip);
    iov[1].iov_base = static_cast<void*>(&udp);
    iov[1].iov_len = sizeof(udp);
    // sendmsg requires these to be non-const but for sending won't modify them
    iov[2].iov_base = static_cast<void*>(const_cast<uint8_t*>(message.data()));
    iov[2].iov_len = message.size();

    struct sockaddr_ll dest;
    memset(&dest, 0, sizeof(dest));
    dest.sll_family = AF_PACKET;
    dest.sll_protocol = htons(ETH_P_IP);
    dest.sll_ifindex = interfaceIndex;
    dest.sll_halen = ETH_ALEN;
    memset(dest.sll_addr, 0xFF, ETH_ALEN);

    struct msghdr header;
    memset(&header, 0, sizeof(header));
    header.msg_name = &dest;
    header.msg_namelen = sizeof(dest);
    header.msg_iov = iov;
    header.msg_iovlen = sizeof(iov) / sizeof(iov[0]);

    ssize_t res = ::sendmsg(mSocketFd, &header, 0);
    if (res == -1) {
        return Result::error("Failed to send message: %s", strerror(errno));
    }
    return Result::success();
}

Result Socket::receiveFromInterface(Message* message,
                                    unsigned int* interfaceIndex) {
    char controlData[CMSG_SPACE(sizeof(struct in_pktinfo))];
    struct msghdr header;
    memset(&header, 0, sizeof(header));
    struct iovec iov;
    iov.iov_base = message->data();
    iov.iov_len = message->capacity();
    header.msg_iov = &iov;
    header.msg_iovlen = 1;
    header.msg_control = &controlData;
    header.msg_controllen = sizeof(controlData);

    ssize_t bytesRead = ::recvmsg(mSocketFd, &header, 0);
    if (bytesRead < 0) {
        return Result::error("Error receiving on socket: %s", strerror(errno));
    }
    message->setSize(static_cast<size_t>(bytesRead));
    if (header.msg_controllen >= sizeof(struct cmsghdr)) {
        for (struct cmsghdr* ctrl = CMSG_FIRSTHDR(&header);
             ctrl;
             ctrl = CMSG_NXTHDR(&header, ctrl)) {
            if (ctrl->cmsg_level == SOL_IP &&
                ctrl->cmsg_type == IP_PKTINFO) {
                auto packetInfo =
                    reinterpret_cast<struct in_pktinfo*>(CMSG_DATA(ctrl));
                *interfaceIndex = packetInfo->ipi_ifindex;
            }
        }
    }
    return Result::success();
}

Result Socket::receiveRawUdp(uint16_t expectedPort,
                             Message* message,
                             bool* isValid) {
    struct iphdr ip;
    struct udphdr udp;

    struct iovec iov[3];
    iov[0].iov_base = &ip;
    iov[0].iov_len = sizeof(ip);
    iov[1].iov_base = &udp;
    iov[1].iov_len = sizeof(udp);
    iov[2].iov_base = message->data();
    iov[2].iov_len = message->capacity();

    ssize_t bytesRead = ::readv(mSocketFd, iov, 3);
    if (bytesRead < 0) {
        return Result::error("Unable to read from socket: %s", strerror(errno));
    }
    if (static_cast<size_t>(bytesRead) < sizeof(ip) + sizeof(udp)) {
        // Not enough bytes to even cover IP and UDP headers
        *isValid = false;
        return Result::success();
    }
    *isValid = ip.version == IPVERSION &&
               ip.ihl == (sizeof(ip) >> 2) &&
               ip.protocol == IPPROTO_UDP &&
               udp.dest == htons(expectedPort);

    message->setSize(bytesRead - sizeof(ip) - sizeof(udp));
    return Result::success();
}

Result Socket::enableOption(int level, int optionName) {
    if (mSocketFd == -1) {
        return Result::error("Socket not open");
    }

    int enabled = 1;
    int status = ::setsockopt(mSocketFd,
                              level,
                              optionName,
                              &enabled,
                              sizeof(enabled));
    if (status == -1) {
        return Result::error("Failed to set socket option: %s",
                             strerror(errno));
    }
    return Result::success();
}
