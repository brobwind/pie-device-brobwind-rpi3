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

#include "timer.h"

#include <time.h>

uint64_t now() {
    struct timespec time = { 0, 0 };
    clock_gettime(CLOCK_MONOTONIC, &time);
    return static_cast<uint64_t>(time.tv_sec) * 1000u +
           static_cast<uint64_t>(time.tv_nsec / 1000000u);
}

Timer::Timer() : mExpires(0) {
}

void Timer::expireSeconds(uint64_t seconds) {
    mExpires = now() + seconds * 1000u;
}

bool Timer::expired() const {
    return now() >= mExpires;
}

uint64_t Timer::remainingMillis() const {
    uint64_t current = now();
    if (current > mExpires) {
        return 0;
    }
    return mExpires - current;
}

