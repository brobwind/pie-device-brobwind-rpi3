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

#include "message.h"
#include "dhcp.h"

#include <string.h>

#include <vector>

static uint32_t sNextTransactionId = 1;

static const ptrdiff_t kOptionOffset = 7;

// The default lease time in seconds
static const uint32_t kDefaultLeaseTime = 10 * 60;

// The parameters that the client would like to receive from the server
static const uint8_t kRequestParameters[] = { OPT_SUBNET_MASK,
                                              OPT_GATEWAY,
                                              OPT_DNS,
                                              OPT_BROADCAST_ADDR,
                                              OPT_LEASE_TIME,
                                              OPT_T1,
                                              OPT_T2,
                                              OPT_MTU };

Message::Message() {
    memset(&dhcpData, 0, sizeof(dhcpData));
    mSize = 0;
}

Message::Message(const uint8_t* data, size_t size) {
    if (size <= sizeof(dhcpData)) {
        memcpy(&dhcpData, data, size);
        mSize = size;
    } else {
        memset(&dhcpData, 0, sizeof(dhcpData));
        mSize = 0;
    }
}

Message Message::discover(const uint8_t (&sourceMac)[ETH_ALEN]) {
    Message message(OP_BOOTREQUEST,
                    sourceMac,
                    static_cast<uint8_t>(DHCPDISCOVER));

    message.addOption(OPT_PARAMETER_LIST, kRequestParameters);
    message.endOptions();

    return message;
}

Message Message::request(const uint8_t (&sourceMac)[ETH_ALEN],
                         in_addr_t requestAddress,
                         in_addr_t serverAddress) {

    Message message(OP_BOOTREQUEST,
                    sourceMac,
                    static_cast<uint8_t>(DHCPREQUEST));

    message.addOption(OPT_PARAMETER_LIST, kRequestParameters);
    message.addOption(OPT_REQUESTED_IP, requestAddress);
    message.addOption(OPT_SERVER_ID, serverAddress);
    message.endOptions();

    return message;
}

Message Message::offer(const Message& sourceMessage,
                       in_addr_t serverAddress,
                       in_addr_t offeredAddress,
                       in_addr_t offeredNetmask,
                       in_addr_t offeredGateway,
                       const in_addr_t* offeredDnsServers,
                       size_t numOfferedDnsServers) {

    uint8_t macAddress[ETH_ALEN];
    memcpy(macAddress, sourceMessage.dhcpData.chaddr, sizeof(macAddress));
    Message message(OP_BOOTREPLY, macAddress, static_cast<uint8_t>(DHCPOFFER));

    message.dhcpData.xid = sourceMessage.dhcpData.xid;
    message.dhcpData.flags = sourceMessage.dhcpData.flags;
    message.dhcpData.yiaddr = offeredAddress;
    message.dhcpData.giaddr = sourceMessage.dhcpData.giaddr;

    message.addOption(OPT_SERVER_ID, serverAddress);
    message.addOption(OPT_LEASE_TIME, kDefaultLeaseTime);
    message.addOption(OPT_SUBNET_MASK, offeredNetmask);
    message.addOption(OPT_GATEWAY, offeredGateway);
    message.addOption(OPT_DNS,
                      offeredDnsServers,
                      numOfferedDnsServers * sizeof(in_addr_t));

    message.endOptions();

    return message;
}

Message Message::ack(const Message& sourceMessage,
                     in_addr_t serverAddress,
                     in_addr_t offeredAddress,
                     in_addr_t offeredNetmask,
                     in_addr_t offeredGateway,
                     const in_addr_t* offeredDnsServers,
                     size_t numOfferedDnsServers) {
    uint8_t macAddress[ETH_ALEN];
    memcpy(macAddress, sourceMessage.dhcpData.chaddr, sizeof(macAddress));
    Message message(OP_BOOTREPLY, macAddress, static_cast<uint8_t>(DHCPACK));

    message.dhcpData.xid = sourceMessage.dhcpData.xid;
    message.dhcpData.flags = sourceMessage.dhcpData.flags;
    message.dhcpData.yiaddr = offeredAddress;
    message.dhcpData.giaddr = sourceMessage.dhcpData.giaddr;

    message.addOption(OPT_SERVER_ID, serverAddress);
    message.addOption(OPT_LEASE_TIME, kDefaultLeaseTime);
    message.addOption(OPT_SUBNET_MASK, offeredNetmask);
    message.addOption(OPT_GATEWAY, offeredGateway);
    message.addOption(OPT_DNS,
                      offeredDnsServers,
                      numOfferedDnsServers * sizeof(in_addr_t));

    message.endOptions();

    return message;
}

Message Message::nack(const Message& sourceMessage, in_addr_t serverAddress) {
    uint8_t macAddress[ETH_ALEN];
    memcpy(macAddress, sourceMessage.dhcpData.chaddr, sizeof(macAddress));
    Message message(OP_BOOTREPLY, macAddress, static_cast<uint8_t>(DHCPNAK));

    message.dhcpData.xid = sourceMessage.dhcpData.xid;
    message.dhcpData.flags = sourceMessage.dhcpData.flags;
    message.dhcpData.giaddr = sourceMessage.dhcpData.giaddr;

    message.addOption(OPT_SERVER_ID, serverAddress);
    message.endOptions();

    return message;
}

bool Message::isValidDhcpMessage(uint8_t expectedOp,
                                 uint32_t expectedXid) const {
    if (!isValidDhcpMessage(expectedOp)) {
        return false;
    }
    // Only look for message with a matching transaction ID
    if (dhcpData.xid != expectedXid) {
        return false;
    }
    return true;
}

bool Message::isValidDhcpMessage(uint8_t expectedOp) const {
    // Require that there is at least enough options for the DHCP cookie
    if (dhcpData.options + 4 > end()) {
        return false;
    }

    if (dhcpData.op != expectedOp) {
        return false;
    }
    if (dhcpData.htype != HTYPE_ETHER) {
        return false;
    }
    if (dhcpData.hlen != ETH_ALEN) {
        return false;
    }

    // Need to have the correct cookie in the options
    if (dhcpData.options[0] != OPT_COOKIE1) {
        return false;
    }
    if (dhcpData.options[1] != OPT_COOKIE2) {
        return false;
    }
    if (dhcpData.options[2] != OPT_COOKIE3) {
        return false;
    }
    if (dhcpData.options[3] != OPT_COOKIE4) {
        return false;
    }

    return true;
}

size_t Message::optionsSize() const {
    auto options = reinterpret_cast<const uint8_t*>(&dhcpData.options);
    const uint8_t* msgEnd = end();
    if (msgEnd <= options) {
        return 0;
    }
    return msgEnd - options;
}

uint8_t Message::type() const {
    uint8_t length = 0;
    const uint8_t* opt = getOption(OPT_MESSAGE_TYPE, &length);
    if (opt && length == 1) {
        return *opt;
    }
    return 0;
}

in_addr_t Message::serverId() const {
    uint8_t length = 0;
    const uint8_t* opt = getOption(OPT_SERVER_ID, &length);
    if (opt && length == 4) {
        return *reinterpret_cast<const in_addr_t*>(opt);
    }
    return 0;
}

in_addr_t Message::requestedIp() const {
    uint8_t length = 0;
    const uint8_t* opt = getOption(OPT_REQUESTED_IP, &length);
    if (opt && length == 4) {
        return *reinterpret_cast<const in_addr_t*>(opt);
    }
    return 0;
}

Message::Message(uint8_t operation,
                 const uint8_t (&macAddress)[ETH_ALEN],
                 uint8_t type) {
    memset(&dhcpData, 0, sizeof(dhcpData));

    dhcpData.op = operation;
    dhcpData.htype = HTYPE_ETHER;
    dhcpData.hlen = ETH_ALEN;
    dhcpData.hops = 0;

    dhcpData.flags = htons(FLAGS_BROADCAST);

    dhcpData.xid = htonl(sNextTransactionId++);

    memcpy(dhcpData.chaddr, macAddress, ETH_ALEN);

    uint8_t* opts = dhcpData.options;

    *opts++ = OPT_COOKIE1;
    *opts++ = OPT_COOKIE2;
    *opts++ = OPT_COOKIE3;
    *opts++ = OPT_COOKIE4;

    *opts++ = OPT_MESSAGE_TYPE;
    *opts++ = 1;
    *opts++ = type;

    updateSize(opts);
}

void Message::addOption(uint8_t type, const void* data, uint8_t size) {
    uint8_t* opts = nextOption();

    *opts++ = type;
    *opts++ = size;
    memcpy(opts, data, size);
    opts += size;

    updateSize(opts);
}

void Message::endOptions() {
    uint8_t* opts = nextOption();

    *opts++ = OPT_END;

    updateSize(opts);
}

const uint8_t* Message::getOption(uint8_t expectedOptCode,
                                  uint8_t* length) const {
    size_t optsSize = optionsSize();
    for (size_t i = 4; i + 2 < optsSize; ) {
        uint8_t optCode = dhcpData.options[i];
        uint8_t optLen = dhcpData.options[i + 1];
        const uint8_t* opt = dhcpData.options + i + 2;

        if (optCode == OPT_END) {
            return nullptr;
        }
        if (optCode == expectedOptCode) {
            *length = optLen;
            return opt;
        }
        i += 2 + optLen;
    }
    return nullptr;
}

uint8_t* Message::nextOption() {
    return reinterpret_cast<uint8_t*>(&dhcpData) + size();
}

void Message::updateSize(uint8_t* optionsEnd) {
    mSize = optionsEnd - reinterpret_cast<uint8_t*>(&dhcpData);
}

