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

#include "dhcpclient.h"
#include "dhcp.h"
#include "interface.h"
#include "log.h"

#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <poll.h>
#include <unistd.h>

#include <cutils/properties.h>

#include <inttypes.h>

// The initial retry timeout for DHCP is 4000 milliseconds
static const uint32_t kInitialTimeout = 4000;
// The maximum retry timeout for DHCP is 64000 milliseconds
static const uint32_t kMaxTimeout = 64000;
// A specific value that indicates that no timeout should happen and that
// the state machine should immediately transition to the next state
static const uint32_t kNoTimeout = 0;

// Enable debug messages
static const bool kDebug = false;

// The number of milliseconds that the timeout should vary (up or down) from the
// base timeout. DHCP requires a -1 to +1 second variation in timeouts.
static const int kTimeoutSpan = 1000;

static std::string addrToStr(in_addr_t address) {
    struct in_addr addr = { address };
    char buffer[64];
    return inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
}

DhcpClient::DhcpClient()
    : mRandomEngine(std::random_device()()),
      mRandomDistribution(-kTimeoutSpan, kTimeoutSpan),
      mState(State::Init),
      mNextTimeout(kInitialTimeout),
      mFuzzNextTimeout(true) {
}

Result DhcpClient::init(const char* interfaceName) {
    Result res = mInterface.init(interfaceName);
    if (!res) {
        return res;
    }

    res = mRouter.init();
    if (!res) {
        return res;
    }

    res = mSocket.open(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
    if (!res) {
        return res;
    }

    res = mSocket.bindRaw(mInterface.getIndex());
    if (!res) {
        return res;
    }
    return Result::success();
}

Result DhcpClient::run() {
    // Block all signals while we're running. This way we don't have to deal
    // with things like EINTR. waitAndReceive then uses ppoll to set the
    // original mask while polling. This way polling can be interrupted but
    // socket writing, reading and ioctl remain interrupt free. If a signal
    // arrives while we're blocking it it will be placed in the signal queue
    // and handled once ppoll sets the original mask. This way no signals are
    // lost.
    sigset_t blockMask, originalMask;
    int status = ::sigfillset(&blockMask);
    if (status != 0) {
        return Result::error("Unable to fill signal set: %s", strerror(errno));
    }
    status = ::sigprocmask(SIG_SETMASK, &blockMask, &originalMask);
    if (status != 0) {
        return Result::error("Unable to set signal mask: %s", strerror(errno));
    }

    for (;;) {
        // Before waiting, polling or receiving we check the current state and
        // see what we should do next. This may result in polling but could
        // also lead to instant state changes without any polling. The new state
        // will then be evaluated instead, most likely leading to polling.
        switch (mState) {
            case State::Init:
                // The starting state. This is the state the client is in when
                // it first starts. It's also the state that the client returns
                // to when things go wrong in other states.
                setNextState(State::Selecting);
                break;
            case State::Selecting:
                // In the selecting state the client attempts to find DHCP
                // servers on the network. The client remains in this state
                // until a suitable server responds.
                sendDhcpDiscover();
                increaseTimeout();
                break;
            case State::Requesting:
                // In the requesting state the client has found a suitable
                // server. The next step is to send a request directly to that
                // server.
                if (mNextTimeout >= kMaxTimeout) {
                    // We've tried to request a bunch of times, start over
                    setNextState(State::Init);
                } else {
                    sendDhcpRequest(mServerAddress);
                    increaseTimeout();
                }
                break;
            case State::Bound:
                // The client enters the bound state when the server has
                // accepted and acknowledged a request and given us a lease. At
                // this point the client will wait until the lease is close to
                // expiring and then it will try to renew the lease.
                if (mT1.expired()) {
                    // Lease expired, renew lease
                    setNextState(State::Renewing);
                } else {
                    // Spurious wake-up, continue waiting. Do not fuzz the
                    // timeout with a random offset. Doing so can cause wakeups
                    // before the timer has expired causing unnecessary
                    // processing. Even worse it can cause the timer to expire
                    // after the lease has ended.
                    mNextTimeout = mT1.remainingMillis();
                    mFuzzNextTimeout = false;
                }
                break;
            case State::Renewing:
                // In the renewing state the client is sending a request for the
                // same address it had was previously bound to. If the second
                // timer expires when in this state the client will attempt to
                // do a full rebind.
                if (mT2.expired()) {
                    // Timeout while renewing, move to rebinding
                    setNextState(State::Rebinding);
                } else {
                    sendDhcpRequest(mServerAddress);
                    increaseTimeout();
                }
                break;
            case State::Rebinding:
                // The client was unable to renew the lease and moved to the
                // rebinding state. In this state the client sends a request for
                // the same address it had before to the broadcast address. This
                // means that any DHCP server on the network is free to respond.
                // After attempting this a few times the client will give up and
                // move to the Init state to try to find a new DHCP server.
                if (mNextTimeout >= kMaxTimeout) {
                    // We've tried to rebind a bunch of times, start over
                    setNextState(State::Init);
                } else {
                    // Broadcast a request
                    sendDhcpRequest(INADDR_BROADCAST);
                    increaseTimeout();
                }
                break;
            default:
                break;
        }
        // The proper action for the current state has been taken, perform any
        // polling and/or waiting needed.
        waitAndReceive(originalMask);
    }

    return Result::error("Client terminated unexpectedly");
}

const char* DhcpClient::stateToStr(State state) {
    switch (state) {
        case State::Init:
            return "Init";
        case State::Selecting:
            return "Selecting";
        case State::Requesting:
            return "Requesting";
        case State::Bound:
            return "Bound";
        case State::Renewing:
            return "Renewing";
        case State::Rebinding:
            return "Rebinding";
    }
    return "<unknown>";
}

void DhcpClient::waitAndReceive(const sigset_t& pollSignalMask) {
    if (mNextTimeout == kNoTimeout) {
        // If there is no timeout the state machine has indicated that it wants
        // an immediate transition to another state. Do nothing.
        return;
    }

    struct pollfd fds;
    fds.fd = mSocket.get();
    fds.events = POLLIN;

    uint32_t timeout = calculateTimeoutMillis();
    for (;;) {
        uint64_t startedAt = now();

        struct timespec ts;
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout - ts.tv_sec * 1000) * 1000000;

        // Poll for any incoming traffic with the calculated timeout. While
        // polling the original signal mask is set so that the polling can be
        // interrupted.
        int res = ::ppoll(&fds, 1, &ts, &pollSignalMask);
        if (res == 0) {
            // Timeout, return to let the caller evaluate
            return;
        } else if (res > 0) {
            // Something to read
            Message msg;
            if (receiveDhcpMessage(&msg)) {
                // We received a DHCP message, check if it's of interest
                uint8_t msgType = msg.type();
                switch (mState) {
                    case State::Selecting:
                        if (msgType == DHCPOFFER) {
                            // Received an offer, move to the Requesting state
                            // to request it.
                            mServerAddress = msg.serverId();
                            mRequestAddress = msg.dhcpData.yiaddr;
                            setNextState(State::Requesting);
                            return;
                        }
                        break;
                    case State::Requesting:
                    case State::Renewing:
                    case State::Rebinding:
                        // All of these states have sent a DHCP request and are
                        // now waiting for an ACK so the behavior is the same.
                        if (msgType == DHCPACK) {
                            // Request approved
                            if (configureDhcp(msg)) {
                                // Successfully configured DHCP, move to Bound
                                setNextState(State::Bound);
                                return;
                            }
                            // Unable to configure DHCP, keep sending requests.
                            // This may not fix the issue but eventually it will
                            // allow for a full timeout which will lead to a
                            // move to the Init state. This might still not fix
                            // the issue but at least the client keeps trying.
                        } else if (msgType == DHCPNAK) {
                            // Request denied, halt network and start over
                            haltNetwork();
                            setNextState(State::Init);
                            return;
                        } 
                        break;
                    default:
                        // For the other states the client is not expecting any
                        // network messages so we ignore those messages.
                        break;
                }
            }
        } else {
            // An error occurred in polling, don't do anything here. The client
            // should keep going anyway to try to acquire a lease in the future
            // if things start working again.
        }
        // If we reach this point we received something that's not a DHCP,
        // message, we timed out, or an error occurred. Go again with whatever
        // time remains.
        uint64_t currentTime = now();
        uint64_t end = startedAt + timeout;
        if (currentTime >= end) {
            // We're done anyway, return and let caller evaluate
            return;
        }
        // Wait whatever the remaining time is
        timeout = end - currentTime;
    }
}

bool DhcpClient::configureDhcp(const Message& msg) {
    uint8_t optLength = 0;

    size_t optsSize = msg.optionsSize();
    if (optsSize < 4) {
        // Message is too small
        if (kDebug) ALOGD("Opts size too small %d", static_cast<int>(optsSize));
        return false;
    }

    const uint8_t* options = msg.dhcpData.options;

    memset(&mDhcpInfo, 0, sizeof(mDhcpInfo));

    // Inspect all options in the message to try to find the ones we want
    for (size_t i = 4; i + 1 < optsSize; ) {
        uint8_t optCode = options[i];
        uint8_t optLength = options[i + 1];
        if (optCode == OPT_END) {
            break;
        }

        if (options + optLength + i >= msg.end()) {
            // Invalid option length, drop it
            if (kDebug) ALOGD("Invalid opt length %d for opt %d",
                              static_cast<int>(optLength),
                              static_cast<int>(optCode));
            return false;
        }
        const uint8_t* opt = options + i + 2;
        switch (optCode) {
            case OPT_LEASE_TIME:
                if (optLength == 4) {
                    mDhcpInfo.leaseTime =
                        ntohl(*reinterpret_cast<const uint32_t*>(opt));
                }
                break;
            case OPT_T1:
                if (optLength == 4) {
                    mDhcpInfo.t1 =
                        ntohl(*reinterpret_cast<const uint32_t*>(opt));
                }
                break;
            case OPT_T2:
                if (optLength == 4) {
                    mDhcpInfo.t2 =
                        ntohl(*reinterpret_cast<const uint32_t*>(opt));
                }
                break;
            case OPT_SUBNET_MASK:
                if (optLength == 4) {
                    mDhcpInfo.subnetMask =
                        *reinterpret_cast<const in_addr_t*>(opt);
                }
                break;
            case OPT_GATEWAY:
                if (optLength >= 4) {
                    mDhcpInfo.gateway =
                        *reinterpret_cast<const in_addr_t*>(opt);
                }
                break;
            case OPT_MTU:
                if (optLength == 2) {
                    mDhcpInfo.mtu =
                        ntohs(*reinterpret_cast<const uint16_t*>(opt));
                }
                break;
            case OPT_DNS:
                if (optLength >= 4) {
                    mDhcpInfo.dns[0] =
                        *reinterpret_cast<const in_addr_t*>(opt);
                }
                if (optLength >= 8) {
                    mDhcpInfo.dns[1] =
                        *reinterpret_cast<const in_addr_t*>(opt + 4);
                }
                if (optLength >= 12) {
                    mDhcpInfo.dns[2] =
                        *reinterpret_cast<const in_addr_t*>(opt + 8);
                }
                if (optLength >= 16) {
                    mDhcpInfo.dns[3] =
                        *reinterpret_cast<const in_addr_t*>(opt + 12);
                }
            case OPT_SERVER_ID:
                if (optLength == 4) {
                    mDhcpInfo.serverId =
                        *reinterpret_cast<const in_addr_t*>(opt);
                }
            default:
                break;
        }
        i += 2 + optLength;
    }
    mDhcpInfo.offeredAddress = msg.dhcpData.yiaddr;

    if (mDhcpInfo.leaseTime == 0) {
        // We didn't get a lease time, ignore this offer
        return false;
    }
    // If there is no T1 or T2 timer given then we create an estimate as
    // suggested for servers in RFC 2131.
    uint32_t t1 = mDhcpInfo.t1, t2 = mDhcpInfo.t2;
    mT1.expireSeconds(t1 > 0 ? t1 : (mDhcpInfo.leaseTime / 2));
    mT2.expireSeconds(t2 > 0 ? t2 : ((mDhcpInfo.leaseTime * 7) / 8));

    Result res = mInterface.bringUp();
    if (!res) {
        ALOGE("Could not configure DHCP: %s", res.c_str());
        return false;
    }

    if (mDhcpInfo.mtu != 0) {
        res = mInterface.setMtu(mDhcpInfo.mtu);
        if (!res) {
            // Consider this non-fatal, the system will not perform at its best
            // but should still work.
            ALOGE("Could not configure DHCP: %s", res.c_str());
        }
    }

    res = mInterface.setAddress(mDhcpInfo.offeredAddress);
    if (!res) {
        ALOGE("Could not configure DHCP: %s", res.c_str());
        return false;
    }

    res = mInterface.setSubnetMask(mDhcpInfo.subnetMask);
    if (!res) {
        ALOGE("Could not configure DHCP: %s", res.c_str());
        return false;
    }

    res = mRouter.setDefaultGateway(mDhcpInfo.gateway, mInterface.getIndex());
    if (!res) {
        ALOGE("Could not configure DHCP: %s", res.c_str());
        return false;
    }
    char propName[64];
    snprintf(propName, sizeof(propName), "net.%s.gw",
             mInterface.getName().c_str());
    property_set(propName, addrToStr(mDhcpInfo.gateway).c_str());

    int numDnsEntries = sizeof(mDhcpInfo.dns) / sizeof(mDhcpInfo.dns[0]);
    for (int i = 0; i < numDnsEntries; ++i) {
        snprintf(propName, sizeof(propName), "net.%s.dns%d",
                 mInterface.getName().c_str(), i + 1);
        if (mDhcpInfo.dns[i] != 0) {
            property_set(propName, addrToStr(mDhcpInfo.dns[i]).c_str());
        } else {
            // Clear out any previous value here in case it was set
            property_set(propName, "");
        }
    }

    return true;
}

void DhcpClient::haltNetwork() {
    Result res = mInterface.setAddress(0);
    if (!res) {
        ALOGE("Could not halt network: %s", res.c_str());
    }
    res = mInterface.bringDown();
    if (!res) {
        ALOGE("Could not halt network: %s", res.c_str());
    }
}

bool DhcpClient::receiveDhcpMessage(Message* msg) {
    bool isValid = false;
    Result res = mSocket.receiveRawUdp(PORT_BOOTP_CLIENT, msg, &isValid);
    if (!res) {
        if (kDebug) ALOGD("Discarding message: %s", res.c_str());
        return false;
    }

    return isValid &&
           msg->isValidDhcpMessage(OP_BOOTREPLY, mLastMsg.dhcpData.xid);
}

uint32_t DhcpClient::calculateTimeoutMillis() {
    if (!mFuzzNextTimeout) {
        return mNextTimeout;
    }
    int adjustment = mRandomDistribution(mRandomEngine);
    if (adjustment < 0 && static_cast<uint32_t>(-adjustment) > mNextTimeout) {
        // Underflow, return a timeout of zero milliseconds
        return 0;
    }
    return mNextTimeout + adjustment;
}

void DhcpClient::increaseTimeout() {
    if (mNextTimeout == kNoTimeout) {
        mNextTimeout = kInitialTimeout;
    } else {
        if (mNextTimeout < kMaxTimeout) {
            mNextTimeout *= 2;
        }
        if (mNextTimeout > kMaxTimeout) {
            mNextTimeout = kMaxTimeout;
        }
    }
}

void DhcpClient::setNextState(State state) {
    if (kDebug) ALOGD("Moving from state %s to %s",
                      stateToStr(mState), stateToStr(state));
    mState = state;
    mNextTimeout = kNoTimeout;
    mFuzzNextTimeout = true;
}

void DhcpClient::sendDhcpRequest(in_addr_t destination) {
    if (kDebug) ALOGD("Sending DHCPREQUEST");
    mLastMsg = Message::request(mInterface.getMacAddress(),
                                mRequestAddress,
                                destination);
    sendMessage(mLastMsg);
}

void DhcpClient::sendDhcpDiscover() {
    if (kDebug) ALOGD("Sending DHCPDISCOVER");
    mLastMsg = Message::discover(mInterface.getMacAddress());
    sendMessage(mLastMsg);
}

void DhcpClient::sendMessage(const Message& message) {
    Result res = mSocket.sendRawUdp(INADDR_ANY,
                                    PORT_BOOTP_CLIENT,
                                    INADDR_BROADCAST,
                                    PORT_BOOTP_SERVER,
                                    mInterface.getIndex(),
                                    message);
    if (!res) {
        ALOGE("Unable to send message: %s", res.c_str());
    }
}

