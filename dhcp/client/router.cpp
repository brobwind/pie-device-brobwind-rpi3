/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "router.h"

#include <linux/rtnetlink.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

template<class Request>
static void addRouterAttribute(Request& r,
                               int type,
                               const void* data,
                               size_t size) {
    // Calculate the offset into the character buffer where the RTA data lives
    // We use offsetof on the buffer to get it. This avoids undefined behavior
    // by casting the buffer (which is safe because it's char) instead of the
    // Request struct.(which is undefined because of aliasing)
    size_t offset = NLMSG_ALIGN(r.hdr.nlmsg_len) - offsetof(Request, buf);
    auto attr = reinterpret_cast<struct rtattr*>(r.buf + offset);
    attr->rta_type = type;
    attr->rta_len = RTA_LENGTH(size);
    memcpy(RTA_DATA(attr), data, size);

    // Update the message length to include the router attribute.
    r.hdr.nlmsg_len = NLMSG_ALIGN(r.hdr.nlmsg_len) + RTA_ALIGN(attr->rta_len);
}

Router::Router() : mSocketFd(-1) {
}

Router::~Router() {
    if (mSocketFd != -1) {
        ::close(mSocketFd);
        mSocketFd = -1;
    }
}

Result Router::init() {
    // Create a netlink socket to the router
    mSocketFd = ::socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (mSocketFd == -1) {
        return Result::error(strerror(errno));
    }
    return Result::success();
}

Result Router::setDefaultGateway(in_addr_t gateway, unsigned int ifaceIndex) {
    struct Request {
        struct nlmsghdr hdr;
        struct rtmsg msg;
        char buf[256];
    } request;

    memset(&request, 0, sizeof(request));

    // Set up a request to create a new route
    request.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(request.msg));
    request.hdr.nlmsg_type = RTM_NEWROUTE;
    request.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;

    request.msg.rtm_family = AF_INET;
    request.msg.rtm_dst_len = 0;
    request.msg.rtm_table = RT_TABLE_MAIN;
    request.msg.rtm_protocol = RTPROT_BOOT;
    request.msg.rtm_scope = RT_SCOPE_UNIVERSE;
    request.msg.rtm_type = RTN_UNICAST;

    addRouterAttribute(request, RTA_GATEWAY, &gateway, sizeof(gateway));
    addRouterAttribute(request, RTA_OIF, &ifaceIndex, sizeof(ifaceIndex));

    return sendNetlinkMessage(&request, request.hdr.nlmsg_len);
}

Result Router::sendNetlinkMessage(const void* data, size_t size) {
    struct sockaddr_nl nlAddress;
    memset(&nlAddress, 0, sizeof(nlAddress));
    nlAddress.nl_family = AF_NETLINK;

    int res = ::sendto(mSocketFd, data, size, 0,
                       reinterpret_cast<sockaddr*>(&nlAddress),
                       sizeof(nlAddress));
    if (res == -1) {
        return Result::error("Unable to send on netlink socket: %s",
                             strerror(errno));
    }
    return Result::success();
}

