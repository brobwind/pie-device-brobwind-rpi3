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
#pragma once

#include "result.h"

#include <linux/if_ether.h>
#include <netinet/in.h>

#include <string>

// A class representing a network interface. The class provides useful
// functionality to configure and query the network interface.
class Interface {
public:
    Interface();
    ~Interface();
    Result init(const char* interfaceName);

    // Returns the interface index indicated by the system
    unsigned int getIndex() const { return mIndex; }
    // Get the MAC address of the interface
    const uint8_t (&getMacAddress() const)[ETH_ALEN] { return mMacAddress; }
    // Get the name of the interface
    const std::string& getName() const { return mInterfaceName; }

    Result bringUp();
    Result bringDown();
    Result setMtu(uint16_t mtu);
    Result setAddress(in_addr_t address);
    Result setSubnetMask(in_addr_t subnetMask);

private:
    struct ifreq createRequest() const;
    Result populateIndex();
    Result populateMacAddress();
    Result setInterfaceUp(bool shouldBeUp);

    std::string mInterfaceName;
    int mSocketFd;
    unsigned int mIndex;
    uint8_t mMacAddress[ETH_ALEN];
};

