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
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>

#include <initializer_list>

class Message {
public:
    Message();
    Message(const uint8_t* data, size_t size);
    static Message discover(const uint8_t (&sourceMac)[ETH_ALEN]);
    static Message request(const uint8_t (&sourceMac)[ETH_ALEN],
                           in_addr_t requestAddress,
                           in_addr_t serverAddress);
    static Message offer(const Message& sourceMessage,
                         in_addr_t serverAddress,
                         in_addr_t offeredAddress,
                         in_addr_t offeredNetmask,
                         in_addr_t offeredGateway,
                         const in_addr_t* offeredDnsServers,
                         size_t numOfferedDnsServers);
    static Message ack(const Message& sourceMessage,
                       in_addr_t serverAddress,
                       in_addr_t offeredAddress,
                       in_addr_t offeredNetmask,
                       in_addr_t offeredGateway,
                       const in_addr_t* offeredDnsServers,
                       size_t numOfferedDnsServers);
    static Message nack(const Message& sourceMessage, in_addr_t serverAddress);

    // Ensure that the data in the message represent a valid DHCP message
    bool isValidDhcpMessage(uint8_t expectedOp) const;
    // Ensure that the data in the message represent a valid DHCP message and
    // has a xid (transaction ID) that matches |expectedXid|.
    bool isValidDhcpMessage(uint8_t expectedOp, uint32_t expectedXid) const;

    const uint8_t* data() const {
        return reinterpret_cast<const uint8_t*>(&dhcpData);
    }
    uint8_t* data() {
        return reinterpret_cast<uint8_t*>(&dhcpData);
    }
    const uint8_t* end() const { return data() + mSize; }

    size_t optionsSize() const;
    size_t size() const { return mSize; }
    void setSize(size_t size) { mSize = size; }
    size_t capacity() const { return sizeof(dhcpData); }

    // Get the DHCP message type
    uint8_t type() const;
    // Get the DHCP server ID
    in_addr_t serverId() const;
    // Get the requested IP
    in_addr_t requestedIp() const;

    struct Dhcp {
        uint8_t op;           /* BOOTREQUEST / BOOTREPLY    */
        uint8_t htype;        /* hw addr type               */
        uint8_t hlen;         /* hw addr len                */
        uint8_t hops;         /* client set to 0            */

        uint32_t xid;         /* transaction id             */

        uint16_t secs;        /* seconds since start of acq */
        uint16_t flags;

        uint32_t ciaddr;      /* client IP addr             */
        uint32_t yiaddr;      /* your (client) IP addr      */
        uint32_t siaddr;      /* ip addr of next server     */
                              /* (DHCPOFFER and DHCPACK)    */
        uint32_t giaddr;      /* relay agent IP addr        */

        uint8_t chaddr[16];  /* client hw addr             */
        char sname[64];      /* asciiz server hostname     */
        char file[128];      /* asciiz boot file name      */

        uint8_t options[1024];  /* optional parameters        */
    }  dhcpData;
private:
    Message(uint8_t operation,
            const uint8_t (&macAddress)[ETH_ALEN],
            uint8_t type);

    void addOption(uint8_t type, const void* data, uint8_t size);
    template<typename T>
    void addOption(uint8_t type, T data) {
        static_assert(sizeof(T) <= 255, "The size of data is too large");
        addOption(type, &data, sizeof(data));
    }
    template<typename T, size_t N>
    void addOption(uint8_t type, T (&items)[N]) {
        static_assert(sizeof(T) * N <= 255,
                      "The size of data is too large");
        uint8_t* opts = nextOption();
        *opts++ = type;
        *opts++ = sizeof(T) * N;
        for (const T& item : items) {
            memcpy(opts, &item, sizeof(item));
            opts += sizeof(item);
        }
        updateSize(opts);
    }
    void endOptions();

    const uint8_t* getOption(uint8_t optCode, uint8_t* length) const;
    uint8_t* nextOption();
    void updateSize(uint8_t* optionsEnd);
    size_t mSize;
};

static_assert(offsetof(Message::Dhcp, htype) == sizeof(Message::Dhcp::op),
              "Invalid packing for DHCP message struct");
