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

#include <stdint.h>

// Return the current timestamp from a monotonic clock in milliseconds.
uint64_t now();

class Timer {
public:
    // Create a timer, initially the timer is already expired.
    Timer();

    // Set the timer to expire in |seconds| seconds.
    void expireSeconds(uint64_t seconds);

    // Return true if the timer has expired.
    bool expired() const;
    // Get the remaining time on the timer in milliseconds.
    uint64_t remainingMillis() const;

private:
    uint64_t mExpires;
};

