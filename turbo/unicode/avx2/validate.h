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
//
// Created by jeff on 23-12-29.
//

#ifndef TURBO_UNICODE_AVX2_VALIDATE_H_
#define TURBO_UNICODE_AVX2_VALIDATE_H_

#include "turbo/unicode/scalar/validate.h"
#include "turbo/unicode/simd/fwd.h"
#include "turbo/base/bits.h"
#include "turbo/unicode/avx2/engine.h"
#include "turbo/unicode/avx2/simd.h"

namespace turbo::unicode::simd {
    /*
    In UTF-16 code units in range 0xD800 to 0xDFFF have special meaning.

    In a vectorized algorithm we want to examine the most significant
    nibble in order to select a fast path. If none of highest nibbles
    are 0xD (13), than we are sure that UTF-16 chunk in a vector
    register is valid.

    Let us analyze what we need to check if the nibble is 0xD. The
    value of the preceding nibble determines what we have:

    0xd000 .. 0xd7ff - a valid word
    0xd800 .. 0xdbff - low surrogate
    0xdc00 .. 0xdfff - high surrogate

    Other constraints we have to consider:
    - there must not be two consecutive low surrogates (0xd800 .. 0xdbff)
    - there must not be two consecutive high surrogates (0xdc00 .. 0xdfff)
    - there must not be sole low surrogate nor high surrogate

    We're going to build three bitmasks based on the 3rd nibble:
    - V = valid word,
    - L = low surrogate (0xd800 .. 0xdbff)
    - H = high surrogate (0xdc00 .. 0xdfff)

      0   1   2   3   4   5   6   7    <--- word index
    [ V | L | H | L | H | V | V | L ]
      1   0   0   0   0   1   1   0     - V = valid masks
      0   1   0   1   0   0   0   1     - L = low surrogate
      0   0   1   0   1   0   0   0     - H high surrogate


      1   0   0   0   0   1   1   0   V = valid masks
      0   1   0   1   0   0   0   0   a = L & (H >> 1)
      0   0   1   0   1   0   0   0   b = a << 1
      1   1   1   1   1   1   1   0   c = V | a | b
                                  ^
                                  the last bit can be zero, we just consume 7 code units
                                  and recheck this word in the next iteration
*/

    /* Returns:
       - pointer to the last unprocessed character (a scalar fallback should check the rest);
       - nullptr if an error was detected.
    */
    template <EndianNess big_endian>
    inline const char16_t* avx2_validate_utf16(const char16_t* input, size_t size) {
        const char16_t* end = input + size;

        const auto v_d8 = simd::simd8<uint8_t,avx2_engine>::splat(0xd8);
        const auto v_f8 = simd::simd8<uint8_t,avx2_engine>::splat(0xf8);
        const auto v_fc = simd::simd8<uint8_t,avx2_engine>::splat(0xfc);
        const auto v_dc = simd::simd8<uint8_t,avx2_engine>::splat(0xdc);

        while (input + simd16<uint16_t, avx2_engine>::ELEMENTS * 2 < end) {
            // 0. Load data: since the validation takes into account only higher
            //    byte of each word, we compress the two vectors into one which
            //    consists only the higher bytes.
            auto in0 = simd16<uint16_t, avx2_engine>(input);
            auto in1 = simd16<uint16_t, avx2_engine>(input + simd16<uint16_t, avx2_engine>::ELEMENTS);

            if (big_endian == EndianNess::SYS_BIG_ENDIAN) {
                in0 = in0.swap_bytes();
                in1 = in1.swap_bytes();
            }

            const auto t0 = in0.shr<8>();
            const auto t1 = in1.shr<8>();

            const auto in = simd16<uint16_t, avx2_engine>::pack(t0, t1);

            // 1. Check whether we have any 0xD800..DFFF word (0b1101'1xxx'yyyy'yyyy).
            const auto surrogates_wordmask = (in & v_f8) == v_d8;
            const uint32_t surrogates_bitmask = surrogates_wordmask.to_bitmask();
            if (surrogates_bitmask == 0x0) {
                input += simd16<uint16_t, avx2_engine>::ELEMENTS * 2;
            } else {
                // 2. We have some surrogates that have to be distinguished:
                //    - low  surrogates: 0b1101'10xx'yyyy'yyyy (0xD800..0xDBFF)
                //    - high surrogates: 0b1101'11xx'yyyy'yyyy (0xDC00..0xDFFF)
                //
                //    Fact: high surrogate has 11th bit set (3rd bit in the higher word)

                // V - non-surrogate code units
                //     V = not surrogates_wordmask
                const uint32_t V = ~surrogates_bitmask;

                // H - word-mask for high surrogates: the six highest bits are 0b1101'11
                const auto    vH = (in & v_fc) == v_dc;
                const uint32_t H = vH.to_bitmask();

                // L - word mask for low surrogates
                //     L = not H and surrogates_wordmask
                const uint32_t L = ~H & surrogates_bitmask;

                const uint32_t a = L & (H >> 1);  // A low surrogate must be followed by high one.
                // (A low surrogate placed in the 7th register's word
                // is an exception we handle.)
                const uint32_t b = a << 1;        // Just mark that the opposite fact is hold,
                // thanks to that we have only two masks for valid case.
                const uint32_t c = V | a | b;     // Combine all the masks into the final one.

                if (c == 0xffffffff) {
                    // The whole input register contains valid UTF-16, i.e.,
                    // either single code units or proper surrogate pairs.
                    input += simd16<uint16_t, avx2_engine>::ELEMENTS * 2;
                } else if (c == 0x7fffffff) {
                    // The 31 lower code units of the input register contains valid UTF-16.
                    // The 31 word may be either a low or high surrogate. It the next
                    // iteration we 1) check if the low surrogate is followed by a high
                    // one, 2) reject sole high surrogate.
                    input += simd16<uint16_t, avx2_engine>::ELEMENTS * 2 - 1;
                } else {
                    return nullptr;
                }
            }
        }

        return input;
    }


    template <EndianNess big_endian>
    inline const UnicodeResult avx2_validate_utf16_with_errors(const char16_t* input, size_t size) {
        const char16_t* start = input;
        const char16_t* end = input + size;

        const auto v_d8 = simd8<uint8_t,avx2_engine>::splat(0xd8);
        const auto v_f8 = simd8<uint8_t,avx2_engine>::splat(0xf8);
        const auto v_fc = simd8<uint8_t,avx2_engine>::splat(0xfc);
        const auto v_dc = simd8<uint8_t,avx2_engine>::splat(0xdc);

        while (input + simd16<uint16_t,avx2_engine>::ELEMENTS * 2 < end) {
            // 0. Load data: since the validation takes into account only higher
            //    byte of each word, we compress the two vectors into one which
            //    consists only the higher bytes.
            auto in0 = simd16<uint16_t,avx2_engine>(input);
            auto in1 = simd16<uint16_t,avx2_engine>(input + simd16<uint16_t,avx2_engine>::ELEMENTS);

            if (big_endian == EndianNess::SYS_BIG_ENDIAN) {
                in0 = in0.swap_bytes();
                in1 = in1.swap_bytes();
            }

            const auto t0 = in0.shr<8>();
            const auto t1 = in1.shr<8>();

            const auto in = simd16<uint16_t,avx2_engine>::pack(t0, t1);

            // 1. Check whether we have any 0xD800..DFFF word (0b1101'1xxx'yyyy'yyyy).
            const auto surrogates_wordmask = (in & v_f8) == v_d8;
            const uint32_t surrogates_bitmask = surrogates_wordmask.to_bitmask();
            if (surrogates_bitmask == 0x0) {
                input += simd16<uint16_t,avx2_engine>::ELEMENTS * 2;
            } else {
                // 2. We have some surrogates that have to be distinguished:
                //    - low  surrogates: 0b1101'10xx'yyyy'yyyy (0xD800..0xDBFF)
                //    - high surrogates: 0b1101'11xx'yyyy'yyyy (0xDC00..0xDFFF)
                //
                //    Fact: high surrogate has 11th bit set (3rd bit in the higher word)

                // V - non-surrogate code units
                //     V = not surrogates_wordmask
                const uint32_t V = ~surrogates_bitmask;

                // H - word-mask for high surrogates: the six highest bits are 0b1101'11
                const auto    vH = (in & v_fc) == v_dc;
                const uint32_t H = vH.to_bitmask();

                // L - word mask for low surrogates
                //     L = not H and surrogates_wordmask
                const uint32_t L = ~H & surrogates_bitmask;

                const uint32_t a = L & (H >> 1);  // A low surrogate must be followed by high one.
                // (A low surrogate placed in the 7th register's word
                // is an exception we handle.)
                const uint32_t b = a << 1;        // Just mark that the opposite fact is hold,
                // thanks to that we have only two masks for valid case.
                const uint32_t c = V | a | b;     // Combine all the masks into the final one.

                if (c == 0xffffffff) {
                    // The whole input register contains valid UTF-16, i.e.,
                    // either single code units or proper surrogate pairs.
                    input += simd16<uint16_t,avx2_engine>::ELEMENTS * 2;
                } else if (c == 0x7fffffff) {
                    // The 31 lower code units of the input register contains valid UTF-16.
                    // The 31 word may be either a low or high surrogate. It the next
                    // iteration we 1) check if the low surrogate is followed by a high
                    // one, 2) reject sole high surrogate.
                    input += simd16<uint16_t,avx2_engine>::ELEMENTS * 2 - 1;
                } else {
                    return UnicodeResult(UnicodeError::SURROGATE, input - start);
                }
            }
        }

        return UnicodeResult(UnicodeError::SUCCESS, input - start);
    }

    /* Returns:
   - pointer to the last unprocessed character (a scalar fallback should check the rest);
   - nullptr if an error was detected.
    */
    inline const char32_t* avx2_validate_utf32le(const char32_t* input, size_t size) {
        const char32_t* end = input + size;

        const __m256i standardmax = _mm256_set1_epi32(0x10ffff);
        const __m256i offset = _mm256_set1_epi32(0xffff2000);
        const __m256i standardoffsetmax = _mm256_set1_epi32(0xfffff7ff);
        __m256i currentmax = _mm256_setzero_si256();
        __m256i currentoffsetmax = _mm256_setzero_si256();

        while (input + 8 < end) {
            const __m256i in = _mm256_loadu_si256((__m256i *)input);
            currentmax = _mm256_max_epu32(in,currentmax);
            currentoffsetmax = _mm256_max_epu32(_mm256_add_epi32(in, offset), currentoffsetmax);
            input += 8;
        }
        __m256i is_zero = _mm256_xor_si256(_mm256_max_epu32(currentmax, standardmax), standardmax);
        if(_mm256_testz_si256(is_zero, is_zero) == 0) {
            return nullptr;
        }

        is_zero = _mm256_xor_si256(_mm256_max_epu32(currentoffsetmax, standardoffsetmax), standardoffsetmax);
        if(_mm256_testz_si256(is_zero, is_zero) == 0) {
            return nullptr;
        }

        return input;
    }


    inline const UnicodeResult avx2_validate_utf32le_with_errors(const char32_t* input, size_t size) {
        const char32_t* start = input;
        const char32_t* end = input + size;

        const __m256i standardmax = _mm256_set1_epi32(0x10ffff);
        const __m256i offset = _mm256_set1_epi32(0xffff2000);
        const __m256i standardoffsetmax = _mm256_set1_epi32(0xfffff7ff);
        __m256i currentmax = _mm256_setzero_si256();
        __m256i currentoffsetmax = _mm256_setzero_si256();

        while (input + 8 < end) {
            const __m256i in = _mm256_loadu_si256((__m256i *)input);
            currentmax = _mm256_max_epu32(in,currentmax);
            currentoffsetmax = _mm256_max_epu32(_mm256_add_epi32(in, offset), currentoffsetmax);

            __m256i is_zero = _mm256_xor_si256(_mm256_max_epu32(currentmax, standardmax), standardmax);
            if(_mm256_testz_si256(is_zero, is_zero) == 0) {
                return UnicodeResult(UnicodeError::TOO_LARGE, input - start);
            }

            is_zero = _mm256_xor_si256(_mm256_max_epu32(currentoffsetmax, standardoffsetmax), standardoffsetmax);
            if(_mm256_testz_si256(is_zero, is_zero) == 0) {
                return UnicodeResult(UnicodeError::SURROGATE, input - start);
            }
            input += 8;
        }

        return UnicodeResult(UnicodeError::SUCCESS, input - start);
    }

}
#endif  // TURBO_UNICODE_AVX2_VALIDATE_H_
