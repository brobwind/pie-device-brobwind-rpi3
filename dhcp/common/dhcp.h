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

// Ports
#define PORT_BOOTP_SERVER 67
#define PORT_BOOTP_CLIENT 68

// Operations
#define OP_BOOTREQUEST 1
#define OP_BOOTREPLY   2

// Flags
#define FLAGS_BROADCAST 0x8000

// Hardware address types
#define HTYPE_ETHER    1

// The first four bytes of options are a cookie to indicate that the payload are
// DHCP options as opposed to some other BOOTP extension.
#define OPT_COOKIE1          0x63
#define OPT_COOKIE2          0x82
#define OPT_COOKIE3          0x53
#define OPT_COOKIE4          0x63

// BOOTP/DHCP options - see RFC 2132
#define OPT_PAD              0

#define OPT_SUBNET_MASK      1     // 4 <ipaddr>
#define OPT_TIME_OFFSET      2     // 4 <seconds>
#define OPT_GATEWAY          3     // 4*n <ipaddr> * n
#define OPT_DNS              6     // 4*n <ipaddr> * n
#define OPT_DOMAIN_NAME      15    // n <domainnamestring>
#define OPT_MTU              26    // 2 <mtu>
#define OPT_BROADCAST_ADDR   28    // 4 <ipaddr>

#define OPT_REQUESTED_IP     50    // 4 <ipaddr>
#define OPT_LEASE_TIME       51    // 4 <seconds>
#define OPT_MESSAGE_TYPE     53    // 1 <msgtype>
#define OPT_SERVER_ID        54    // 4 <ipaddr>
#define OPT_PARAMETER_LIST   55    // n <optcode> * n
#define OPT_MESSAGE          56    // n <errorstring>
#define OPT_T1               58    // 4 <renewal time value>
#define OPT_T2               59    // 4 <rebinding time value>
#define OPT_CLASS_ID         60    // n <opaque>
#define OPT_CLIENT_ID        61    // n <opaque>
#define OPT_END              255

// DHCP message types
#define DHCPDISCOVER         1
#define DHCPOFFER            2
#define DHCPREQUEST          3
#define DHCPDECLINE          4
#define DHCPACK              5
#define DHCPNAK              6
#define DHCPRELEASE          7
#define DHCPINFORM           8


