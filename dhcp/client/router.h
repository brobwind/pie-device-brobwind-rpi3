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
#pragma once

#include <stdint.h>

#include <netinet/in.h>

#include "result.h"

class Router {
public:
    Router();
    ~Router();
    // Initialize the router, this has to be called before any other methods can
    // be called. It only needs to be called once.
    Result init();

    // Set the default route to |gateway| on the interface specified by
    // |interfaceIndex|. If the default route is already set up with the same
    // configuration then nothing is done. If another default route exists it
    // will be removed and replaced by the new one. If no default route exists
    // a route will be created with the given parameters.
    Result setDefaultGateway(in_addr_t gateway, unsigned int interfaceIndex);
private:
    Result sendNetlinkMessage(const void* data, size_t size);

    // Netlink socket for setting up neighbors and routes
    int mSocketFd;
};

