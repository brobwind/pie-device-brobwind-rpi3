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

#include "interface.h"

#include <errno.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/route.h>
#include <string.h>
#include <unistd.h>

Interface::Interface() : mSocketFd(-1) {
}

Interface::~Interface() {
    if (mSocketFd != -1) {
        close(mSocketFd);
        mSocketFd = -1;
    }
}

Result Interface::init(const char* interfaceName) {
    mInterfaceName = interfaceName;

    if (mSocketFd != -1) {
        return Result::error("Interface initialized more than once");
    }

    mSocketFd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_IP);
    if (mSocketFd == -1) {
        return Result::error("Failed to create interface socket for '%s': %s",
                             interfaceName, strerror(errno));
    }

    Result res = populateIndex();
    if (!res) {
        return res;
    }

    res = populateMacAddress();
    if (!res) {
        return res;
    }

    res = bringUp();
    if (!res) {
        return res;
    }

    res = setAddress(0);
    if (!res) {
        return res;
    }

    return Result::success();
}

Result Interface::bringUp() {
    return setInterfaceUp(true);
}

Result Interface::bringDown() {
    return setInterfaceUp(false);
}

Result Interface::setMtu(uint16_t mtu) {
    struct ifreq request = createRequest();

    strncpy(request.ifr_name, mInterfaceName.c_str(), sizeof(request.ifr_name));
    request.ifr_mtu = mtu;
    int status = ::ioctl(mSocketFd, SIOCSIFMTU, &request);
    if (status != 0) {
        return Result::error("Failed to set interface MTU %u for '%s': %s",
                             static_cast<unsigned int>(mtu),
                             mInterfaceName.c_str(),
                             strerror(errno));
    }

    return Result::success();
}

Result Interface::setAddress(in_addr_t address) {
    struct ifreq request = createRequest();

    auto requestAddr = reinterpret_cast<struct sockaddr_in*>(&request.ifr_addr);
    requestAddr->sin_family = AF_INET;
    requestAddr->sin_port = 0;
    requestAddr->sin_addr.s_addr = address;

    int status = ::ioctl(mSocketFd, SIOCSIFADDR, &request);
    if (status != 0) {
        return Result::error("Failed to set interface address for '%s': %s",
                             mInterfaceName.c_str(), strerror(errno));
    }

    return Result::success();
}

Result Interface::setSubnetMask(in_addr_t subnetMask) {
    struct ifreq request = createRequest();

    auto addr = reinterpret_cast<struct sockaddr_in*>(&request.ifr_addr);
    addr->sin_family = AF_INET;
    addr->sin_port = 0;
    addr->sin_addr.s_addr = subnetMask;

    int status = ::ioctl(mSocketFd, SIOCSIFNETMASK, &request);
    if (status != 0) {
        return Result::error("Failed to set subnet mask for '%s': %s",
                             mInterfaceName.c_str(), strerror(errno));
    }

    return Result::success();
}

struct ifreq Interface::createRequest() const {
    struct ifreq request;
    memset(&request, 0, sizeof(request));
    strncpy(request.ifr_name, mInterfaceName.c_str(), sizeof(request.ifr_name));
    request.ifr_name[sizeof(request.ifr_name) - 1] = '\0';

    return request;
}

Result Interface::populateIndex() {
    struct ifreq request = createRequest();

    int status = ::ioctl(mSocketFd, SIOCGIFINDEX, &request);
    if (status != 0) {
        return Result::error("Failed to get interface index for '%s': %s",
                             mInterfaceName.c_str(), strerror(errno));
    }
    mIndex = request.ifr_ifindex;
    return Result::success();
}

Result Interface::populateMacAddress() {
    struct ifreq request = createRequest();

    int status = ::ioctl(mSocketFd, SIOCGIFHWADDR, &request);
    if (status != 0) {
        return Result::error("Failed to get MAC address for '%s': %s",
                             mInterfaceName.c_str(), strerror(errno));
    }
    memcpy(mMacAddress, &request.ifr_hwaddr.sa_data, ETH_ALEN);
    return Result::success();
}

Result Interface::setInterfaceUp(bool shouldBeUp) {
    struct ifreq request = createRequest();

    int status = ::ioctl(mSocketFd, SIOCGIFFLAGS, &request);
    if (status != 0) {
        return Result::error("Failed to get interface flags for '%s': %s",
                             mInterfaceName.c_str(), strerror(errno));
    }

    bool isUp = (request.ifr_flags & IFF_UP) != 0;
    if (isUp != shouldBeUp) {
        // Toggle the up flag
        request.ifr_flags ^= IFF_UP;
    } else {
        // Interface is already in desired state, do nothing
        return Result::success();
    }

    status = ::ioctl(mSocketFd, SIOCSIFFLAGS, &request);
    if (status != 0) {
        return Result::error("Failed to set interface flags for '%s': %s",
                             mInterfaceName.c_str(), strerror(errno));
    }

    return Result::success();
}

