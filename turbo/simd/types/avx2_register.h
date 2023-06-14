// Copyright 2023 The titan-search Authors.
// Copyright (c) Jeff.li
// Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and Martin Renou
// Copyright (c) QuantStack
// Copyright (c) Serge Guelton
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


#ifndef TURBO_SIMD_TYPES_AVX2_REGISTER_H_
#define TURBO_SIMD_TYPES_AVX2_REGISTER_H_

#include "turbo/simd/types/avx_register.h"

namespace turbo::simd {
    /**
     * @ingroup architectures
     *
     * AVX2 instructions
     */
    struct avx2 : avx {
        static constexpr bool supported() noexcept { return TURBO_WITH_AVX2; }

        static constexpr bool available() noexcept { return true; }

        static constexpr unsigned version() noexcept { return generic::version(2, 2, 0); }

        static constexpr char const *name() noexcept { return "avx2"; }
    };

#if TURBO_WITH_AVX2
    namespace types
    {
        TURBO_SIMD_DECLARE_SIMD_REGISTER_ALIAS(avx2, avx);
    }
#endif
}

#endif  // TURBO_SIMD_TYPES_AVX2_REGISTER_H_

