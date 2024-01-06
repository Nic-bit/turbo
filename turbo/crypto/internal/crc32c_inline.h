// Copyright 2022 The Turbo Authors.
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

#ifndef TURBO_CRYPTO_INTERNAL_CRC32C_INLINE_H_
#define TURBO_CRYPTO_INTERNAL_CRC32C_INLINE_H_

#include <cstdint>

#include "turbo/base/endian.h"
#include "turbo/crypto/internal/crc32_x86_arm_combined_simd.h"
#include "turbo/platform/port.h"

namespace turbo::crc_internal {

    // CRC32C implementation optimized for small inputs.
    // Either computes crc and return true, or if there is
    // no hardware support does nothing and returns false.
    inline bool ExtendCrc32cInline(uint32_t *crc, const char *p, size_t n) {
#if defined(TURBO_CRYPTO_INTERNAL_HAVE_ARM_SIMD) || \
    defined(TURBO_CRYPTO_INTERNAL_HAVE_X86_SIMD)
        constexpr uint32_t kCrc32Xor = 0xffffffffU;
        *crc ^= kCrc32Xor;
        if (n & 1) {
          *crc = CRC32_u8(*crc, static_cast<uint8_t>(*p));
          n--;
          p++;
        }
        if (n & 2) {
          *crc = CRC32_u16(*crc, turbo::little_endian::load16(p));
          n -= 2;
          p += 2;
        }
        if (n & 4) {
          *crc = CRC32_u32(*crc, turbo::little_endian::load32(p));
          n -= 4;
          p += 4;
        }
        while (n) {
          *crc = CRC32_u64(*crc, turbo::little_endian::load64(p));
          n -= 8;
          p += 8;
        }
        *crc ^= kCrc32Xor;
        return true;
#else
        // No hardware support, signal the need to fallback.
        static_cast<void>(crc);
        static_cast<void>(p);
        static_cast<void>(n);
        return false;
#endif  // defined(TURBO_CRYPTO_INTERNAL_HAVE_ARM_SIMD) ||
        // defined(TURBO_CRYPTO_INTERNAL_HAVE_X86_SIMD)
    }

}  // namespace turbo::crc_internal

#endif  // TURBO_CRYPTO_INTERNAL_CRC32C_INLINE_H_
