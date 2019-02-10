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

#include "interface.h"
#include "message.h"
#include "result.h"
#include "router.h"
#include "socket.h"
#include "timer.h"

#include <netinet/in.h>
#include <stdint.h>

#include <random>


class DhcpClient {
public:
    DhcpClient();

    // Initialize the DHCP client to listen on |interfaceName|.
    Result init(const char* interfaceName);
    Result run();
private:
    enum class State {
        Init,
        Selecting,
        Requesting,
        Bound,
        Renewing,
        Rebinding
    };
    const char* stateToStr(State state);

    // Wait for any pending timeouts
    void waitAndReceive(const sigset_t& pollSignalMask);
    // Create a varying timeout (+- 1 second) based on the next timeout.
    uint32_t calculateTimeoutMillis();
    // Increase the next timeout in a manner that's compliant with the DHCP RFC.
    void increaseTimeout();
    // Move to |state|, the next poll timeout will be zero and the new
    // state will be immediately evaluated.
    void setNextState(State state);
    // Configure network interface based on the DHCP configuration in |msg|.
    bool configureDhcp(const Message& msg);
    // Halt network operations on the network interface for when configuration
    // is not possible and the protocol demands it.
    void haltNetwork();
    // Receive a message on the socket and populate |msg| with the received
    // data. If the message is a valid DHCP message the method returns true. If
    // it's not valid false is returned.
    bool receiveDhcpMessage(Message* msg);

    void sendDhcpDiscover();
    void sendDhcpRequest(in_addr_t destination);
    void sendMessage(const Message& message);
    Result send(in_addr_t source, in_addr_t destination,
                uint16_t sourcePort, uint16_t destinationPort,
                const uint8_t* data, size_t size);

    std::mt19937 mRandomEngine; // Mersenne Twister RNG
    std::uniform_int_distribution<int> mRandomDistribution;

    struct DhcpInfo {
        uint32_t t1;
        uint32_t t2;
        uint32_t leaseTime;
        uint16_t mtu;
        in_addr_t dns[4];
        in_addr_t gateway;
        in_addr_t subnetMask;
        in_addr_t serverId;
        in_addr_t offeredAddress;
    } mDhcpInfo;

    Router mRouter;
    Interface mInterface;
    Message mLastMsg;
    Timer mT1, mT2;
    Socket mSocket;
    State mState;
    uint32_t mNextTimeout;
    bool mFuzzNextTimeout;

    in_addr_t mRequestAddress; // Address we'd like to use in requests
    in_addr_t mServerAddress;  // Server to send request to
};

