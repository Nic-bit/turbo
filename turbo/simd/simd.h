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

#ifndef TURBO_SIMD_H_
#define TURBO_SIMD_H_

#include "turbo/platform/port.h"

#if !TURBO_WITH_AVX2
#warning avx2 not enabled for turbo simd
#endif

#include "turbo/simd/arch/scalar.h"
#include "turbo/simd/memory/aligned_allocator.h"

#if defined(TURBO_NO_SUPPORTED_ARCHITECTURE)
// to type definition or anything appart from scalar definition and aligned allocator
#else

#include "turbo/simd/types/batch.h"
#include "turbo/simd/types/batch_constant.h"
#include "turbo/simd/types/simd_traits.h"

// This include must come last
#include "turbo/simd/types/api.h"

#endif
#endif  // TURBO_SIMD_H_

