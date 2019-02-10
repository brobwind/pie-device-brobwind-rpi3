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
#include "log.h"

static void usage(const char* program) {
    ALOGE("Usage: %s -i <interface>", program);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    const char* interfaceName = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0) {
            if (i + 1 < argc) {
                interfaceName = argv[++i];
            } else {
                ALOGE("ERROR: -i parameter needs an argument");
                usage(argv[0]);
                return 1;
            }
        } else {
            ALOGE("ERROR: unknown parameters %s", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }
    if (interfaceName == nullptr) {
        ALOGE("ERROR: No interface specified");
        usage(argv[0]);
        return 1;
    }

    DhcpClient client;
    Result res = client.init(interfaceName);
    if (!res) {
        ALOGE("Failed to initialize DHCP client: %s\n", res.c_str());
        return 1;
    }

    res = client.run();
    if (!res) {
        ALOGE("DHCP client failed: %s\n", res.c_str());
        return 1;
    }
    // This is weird and shouldn't happen, the client should run forever.
    return 0;
}

