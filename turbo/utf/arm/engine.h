// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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

#ifndef TURBO_UTF_ARM_ENGINE_H_
#define TURBO_UTF_ARM_ENGINE_H_

#include <cstddef>
#include "turbo/utf/fwd.h"

namespace turbo {

    struct UtfArmEngine {
        static constexpr bool supported() noexcept { return true; }

        static constexpr bool available() noexcept { return true; }

        static constexpr unsigned version() noexcept { return 0; }

        static constexpr std::size_t alignment() noexcept { return 0; }

        static constexpr bool requires_alignment() noexcept { return false; }

        static constexpr char const *name() noexcept { return "scalar"; }
    };

    template<>
    struct is_utf_engine<UtfArmEngine> : std::true_type {};

}  // namespace turbo

#endif  // TURBO_UTF_ARM_ENGINE_H_
