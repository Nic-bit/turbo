// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef TURBO_MODULE_CONSTANTS_H_
#define TURBO_MODULE_CONSTANTS_H_

#include "turbo/platform/port.h"
#include "turbo/module/module_version.h"

namespace turbo {

#ifdef TURBO_PLATFORM_WINDOWS
    static const char* kModuleSuffix = ".dll";
    static const char* kModulePrefix = "lib";
#elif defined(TURBO_PLATFORM_LINUX)
    static const char* kModuleSuffix = ".so";
    static const char* kModulePrefix = "lib";
#elif defined(TURBO_PLATFORM_OSX)
    static const char* kModuleSuffix = ".dylib";
    static const char* kModulePrefix = "lib";
#else
#error unknown platform
#endif

    static constexpr ModuleVersion kNullVersion{0,0,0};

}  // namespace turbo

#endif  // TURBO_MODULE_CONSTANTS_H_
