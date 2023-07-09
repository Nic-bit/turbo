// Copyright 2023 The titan-search Authors.
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

#ifndef TURBO_SIMD_TYPES_SSSE3_REGISTER_H_
#define TURBO_SIMD_TYPES_SSSE3_REGISTER_H_

#include "turbo/simd/types/sse3_register.h"

#if TURBO_WITH_SSSE3

#include <tmmintrin.h>

#endif

namespace turbo::simd {
    /**
     * @ingroup architectures
     *
     * SSSE3 instructions
     */
    struct ssse3 : sse3 {
        static constexpr bool supported() noexcept { return TURBO_WITH_SSSE3; }

        static constexpr bool available() noexcept { return true; }

        static constexpr unsigned version() noexcept { return generic::version(1, 3, 1); }

        static constexpr char const *name() noexcept { return "ssse3"; }
    };

#if TURBO_WITH_SSSE3
    namespace types {
        TURBO_SIMD_DECLARE_SIMD_REGISTER_ALIAS(ssse3, sse3);
    }
#endif
}

#endif // TURBO_SIMD_TYPES_SSSE3_REGISTER_H_
