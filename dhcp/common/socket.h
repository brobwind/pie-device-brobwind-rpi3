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

#include <arpa/inet.h>

class Message;

class Socket {
public:
    Socket();
    Socket(const Socket&) = delete;
    ~Socket();

    Socket& operator=(const Socket&) = delete;

    int get() const { return mSocketFd; }
    // Open a socket, |domain|, |type| and |protocol| are as described in the
    // man pages for socket.
    Result open(int domain, int type, int protocol);
    // Bind to a generic |sockaddr| of size |sockaddrLength|
    Result bind(const void* sockaddr, size_t sockaddrLength);
    // Bind to an IP |address| and |port|
    Result bindIp(in_addr_t address, uint16_t port);
    // Bind a raw socket to the interface with index |interfaceIndex|.
    Result bindRaw(unsigned int interfaceIndex);
    // Send data in |message| on an IP socket to
    // |destinationAddress|:|destinationPort|, the message will egress on the
    // interface specified by |interfaceIndex|
    Result sendOnInterface(unsigned int interfaceIndex,
                           in_addr_t destinationAddress,
                           uint16_t destinationPort,
                           const Message& message);
    // Send |message| as a UDP datagram on a raw socket. The source address of
    // the message will be |source|:|sourcePort| and the destination will be
    // |destination|:|destinationPort|. The message will be sent on the
    // interface indicated by |interfaceIndex|.
    Result sendRawUdp(in_addr_t source,
                      uint16_t sourcePort,
                      in_addr_t destination,
                      uint16_t destinationPort,
                      unsigned int interfaceIndex,
                      const Message& message);
    // Receive data on the socket and indicate which interface the data was
    // received on in |interfaceIndex|. The received data is placed in |message|
    Result receiveFromInterface(Message* message, unsigned int* interfaceIndex);
    // Receive UDP data on a raw socket. Expect that the protocol in the IP
    // header is UDP and that the port in the UDP header is |expectedPort|. If
    // the received data is valid then |isValid| will be set to true, otherwise
    // false. The validity check includes the expected values as well as basic
    // size requirements to fit the expected protocol headers.  The method will
    // only return an error result if the actual receiving fails.
    Result receiveRawUdp(uint16_t expectedPort,
                         Message* message,
                         bool* isValid);
    // Enable |optionName| on option |level|. These values are the same as used
    // in setsockopt calls.
    Result enableOption(int level, int optionName);
private:
    int mSocketFd;
};

