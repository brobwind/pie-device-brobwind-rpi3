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

#include <stdio.h>
#include <stdarg.h>

#include <string>

class Result {
public:
    static Result success() {
        return Result(true);
    }
    // Construct a result indicating an error.
    static Result error(std::string message) {
        return Result(message);
    }
    static Result error(const char* format, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        buffer[sizeof(buffer) - 1] = '\0';
        return Result(std::string(buffer));
    }

    bool isSuccess() const { return mSuccess; }
    bool operator!() const { return !mSuccess; }

    const char* c_str() const { return mMessage.c_str(); }
private:
    explicit Result(bool success) : mSuccess(success) { }
    explicit Result(std::string message)
        : mMessage(message), mSuccess(false) {
    }
    std::string mMessage;
    bool mSuccess;
};

