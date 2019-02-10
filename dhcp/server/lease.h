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

#include <linux/if_ether.h>
#include <stdint.h>

#include <functional>

// A lease consists of both the interface index and the MAC address. This
// way the server can run on many different interfaces that have the same
// client MAC address without giving out the same IP address. The reason
// this is useful is because we might have several virtual interfaces, one
// for each access point, that all have the same endpoint on the other side.
// This endpoint would then have the same MAC address and always get the
// same address. But for routing purposes it's useful to give it different
// addresses depending on the server side interface. That way the routing
// table can be set up so that packets are forwarded to the correct access
// point interface based on IP address.
struct Lease {
    Lease(unsigned int interfaceIndex, const uint8_t* macAddress) {
        InterfaceIndex = interfaceIndex;
        memcpy(MacAddress, macAddress, sizeof(MacAddress));
    }
    unsigned int InterfaceIndex;
    uint8_t MacAddress[ETH_ALEN];
};

template<class T>
inline void hash_combine(size_t& seed, const T& value) {
    std::hash<T> hasher;
    seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {
template<> struct hash<Lease> {
    size_t operator()(const Lease& lease) const {
        size_t seed = 0;
        hash_combine(seed, lease.InterfaceIndex);
        // Treat the first 4 bytes as an uint32_t to save some computation
        hash_combine(seed, *reinterpret_cast<const uint32_t*>(lease.MacAddress));
        // And the remaining 2 bytes as an uint16_t
        hash_combine(seed,
                     *reinterpret_cast<const uint16_t*>(lease.MacAddress + 4));
        return seed;
    }
};
}

inline bool operator==(const Lease& left, const Lease& right) {
    return left.InterfaceIndex == right.InterfaceIndex &&
        memcmp(left.MacAddress, right.MacAddress, sizeof(left.MacAddress)) == 0;
}
