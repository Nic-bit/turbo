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

#ifndef TURBO_UNICODE_SCALAR_UTF16_CONVERT_H_
#define TURBO_UNICODE_SCALAR_UTF16_CONVERT_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "turbo/unicode/encoding_types.h"
#include "turbo/unicode/error.h"
#include "turbo/base/endian.h"

namespace turbo::unicode::utf16_to_utf8 {

    template<EndianNess big_endian>
    inline size_t convert(const char16_t *buf, size_t len, char *utf8_output) {
        const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
        size_t pos = 0;
        char *start{utf8_output};
        while (pos < len) {
            // try to convert the next block of 8 ASCII characters
            if (pos + 4 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v;
                ::memcpy(&v, data + pos, sizeof(uint64_t));
                if (!match_system(big_endian)) v = (v >> 8) | (v << (64 - 8));
                if ((v & 0xFF80FF80FF80FF80) == 0) {
                    size_t final_pos = pos + 4;
                    while (pos < final_pos) {
                        *utf8_output++ = !match_system(big_endian) ? char(gbswap_16(buf[pos])) : char(buf[pos]);
                        pos++;
                    }
                    continue;
                }
            }
            uint16_t word = !match_system(big_endian) ? gbswap_16(data[pos]) : data[pos];
            if ((word & 0xFF80) == 0) {
                // will generate one UTF-8 bytes
                *utf8_output++ = char(word);
                pos++;
            } else if ((word & 0xF800) == 0) {
                // will generate two UTF-8 bytes
                // we have 0b110XXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 6) | 0b11000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else if ((word & 0xF800) != 0xD800) {
                // will generate three UTF-8 bytes
                // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 12) | 0b11100000);
                *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else {
                // must be a surrogate pair
                if (pos + 1 >= len) { return 0; }
                uint16_t diff = uint16_t(word - 0xD800);
                if (diff > 0x3FF) { return 0; }
                uint16_t next_word = !match_system(big_endian) ? gbswap_16(data[pos + 1]) : data[pos + 1];
                uint16_t diff2 = uint16_t(next_word - 0xDC00);
                if (diff2 > 0x3FF) { return 0; }
                uint32_t value = (diff << 10) + diff2 + 0x10000;
                // will generate four UTF-8 bytes
                // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
                *utf8_output++ = char((value >> 18) | 0b11110000);
                *utf8_output++ = char(((value >> 12) & 0b111111) | 0b10000000);
                *utf8_output++ = char(((value >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((value & 0b111111) | 0b10000000);
                pos += 2;
            }
        }
        return utf8_output - start;
    }

    template<EndianNess big_endian>
    inline result convert_with_errors(const char16_t *buf, size_t len, char *utf8_output) {
        const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
        size_t pos = 0;
        char *start{utf8_output};
        while (pos < len) {
            // try to convert the next block of 8 ASCII characters
            if (pos + 4 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v;
                ::memcpy(&v, data + pos, sizeof(uint64_t));
                if (!match_system(big_endian)) v = (v >> 8) | (v << (64 - 8));
                if ((v & 0xFF80FF80FF80FF80) == 0) {
                    size_t final_pos = pos + 4;
                    while (pos < final_pos) {
                        *utf8_output++ = !match_system(big_endian) ? char(gbswap_16(buf[pos])) : char(buf[pos]);
                        pos++;
                    }
                    continue;
                }
            }
            uint16_t word = !match_system(big_endian) ? gbswap_16(data[pos]) : data[pos];
            if ((word & 0xFF80) == 0) {
                // will generate one UTF-8 bytes
                *utf8_output++ = char(word);
                pos++;
            } else if ((word & 0xF800) == 0) {
                // will generate two UTF-8 bytes
                // we have 0b110XXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 6) | 0b11000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else if ((word & 0xF800) != 0xD800) {
                // will generate three UTF-8 bytes
                // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 12) | 0b11100000);
                *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else {
                // must be a surrogate pair
                if (pos + 1 >= len) { return result(error_code::SURROGATE, pos); }
                uint16_t diff = uint16_t(word - 0xD800);
                if (diff > 0x3FF) { return result(error_code::SURROGATE, pos); }
                uint16_t next_word = !match_system(big_endian) ? gbswap_16(data[pos + 1]) : data[pos + 1];
                uint16_t diff2 = uint16_t(next_word - 0xDC00);
                if (diff2 > 0x3FF) { return result(error_code::SURROGATE, pos); }
                uint32_t value = (diff << 10) + diff2 + 0x10000;
                // will generate four UTF-8 bytes
                // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
                *utf8_output++ = char((value >> 18) | 0b11110000);
                *utf8_output++ = char(((value >> 12) & 0b111111) | 0b10000000);
                *utf8_output++ = char(((value >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((value & 0b111111) | 0b10000000);
                pos += 2;
            }
        }
        return result(error_code::SUCCESS, utf8_output - start);
    }


    template<EndianNess big_endian>
    inline size_t convert_valid(const char16_t *buf, size_t len, char *utf8_output) {
        const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
        size_t pos = 0;
        char *start{utf8_output};
        while (pos < len) {
            // try to convert the next block of 4 ASCII characters
            if (pos + 4 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v;
                ::memcpy(&v, data + pos, sizeof(uint64_t));
                if (!match_system(big_endian)) v = (v >> 8) | (v << (64 - 8));
                if ((v & 0xFF80FF80FF80FF80) == 0) {
                    size_t final_pos = pos + 4;
                    while (pos < final_pos) {
                        *utf8_output++ = !match_system(big_endian) ? char(gbswap_16(buf[pos])) : char(buf[pos]);
                        pos++;
                    }
                    continue;
                }
            }

            uint16_t word = !match_system(big_endian) ? gbswap_16(data[pos]) : data[pos];
            if ((word & 0xFF80) == 0) {
                // will generate one UTF-8 bytes
                *utf8_output++ = char(word);
                pos++;
            } else if ((word & 0xF800) == 0) {
                // will generate two UTF-8 bytes
                // we have 0b110XXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 6) | 0b11000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else if ((word & 0xF800) != 0xD800) {
                // will generate three UTF-8 bytes
                // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 12) | 0b11100000);
                *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else {
                // must be a surrogate pair
                uint16_t diff = uint16_t(word - 0xD800);
                if (pos + 1 >= len) { return 0; } // minimal bound checking
                uint16_t next_word = !match_system(big_endian) ? gbswap_16(data[pos + 1]) : data[pos + 1];
                uint16_t diff2 = uint16_t(next_word - 0xDC00);
                uint32_t value = (diff << 10) + diff2 + 0x10000;
                // will generate four UTF-8 bytes
                // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
                *utf8_output++ = char((value >> 18) | 0b11110000);
                *utf8_output++ = char(((value >> 12) & 0b111111) | 0b10000000);
                *utf8_output++ = char(((value >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((value & 0b111111) | 0b10000000);
                pos += 2;
            }
        }
        return utf8_output - start;
    }
}  // namespace turbo::unicode::utf16_to_utf8

namespace turbo::unicode::utf16_to_utf32 {

    template<EndianNess big_endian>
    inline size_t convert(const char16_t *buf, size_t len, char32_t *utf32_output) {
        const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
        size_t pos = 0;
        char32_t *start{utf32_output};
        while (pos < len) {
            uint16_t word = !match_system(big_endian) ? gbswap_16(data[pos]) : data[pos];
            if ((word & 0xF800) != 0xD800) {
                // No surrogate pair, extend 16-bit word to 32-bit word
                *utf32_output++ = char32_t(word);
                pos++;
            } else {
                // must be a surrogate pair
                uint16_t diff = uint16_t(word - 0xD800);
                if (diff > 0x3FF) { return 0; }
                if (pos + 1 >= len) { return 0; } // minimal bound checking
                uint16_t next_word = !match_system(big_endian) ? gbswap_16(data[pos + 1]) : data[pos + 1];
                uint16_t diff2 = uint16_t(next_word - 0xDC00);
                if (diff2 > 0x3FF) { return 0; }
                uint32_t value = (diff << 10) + diff2 + 0x10000;
                *utf32_output++ = char32_t(value);
                pos += 2;
            }
        }
        return utf32_output - start;
    }

    template<EndianNess big_endian>
    inline result convert_with_errors(const char16_t *buf, size_t len, char32_t *utf32_output) {
        const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
        size_t pos = 0;
        char32_t *start{utf32_output};
        while (pos < len) {
            uint16_t word = !match_system(big_endian) ? gbswap_16(data[pos]) : data[pos];
            if ((word & 0xF800) != 0xD800) {
                // No surrogate pair, extend 16-bit word to 32-bit word
                *utf32_output++ = char32_t(word);
                pos++;
            } else {
                // must be a surrogate pair
                uint16_t diff = uint16_t(word - 0xD800);
                if (diff > 0x3FF) { return result(error_code::SURROGATE, pos); }
                if (pos + 1 >= len) { return result(error_code::SURROGATE, pos); } // minimal bound checking
                uint16_t next_word = !match_system(big_endian) ? gbswap_16(data[pos + 1]) : data[pos + 1];
                uint16_t diff2 = uint16_t(next_word - 0xDC00);
                if (diff2 > 0x3FF) { return result(error_code::SURROGATE, pos); }
                uint32_t value = (diff << 10) + diff2 + 0x10000;
                *utf32_output++ = char32_t(value);
                pos += 2;
            }
        }
        return result(error_code::SUCCESS, utf32_output - start);
    }


    template<EndianNess big_endian>
    inline size_t convert_valid(const char16_t *buf, size_t len, char32_t *utf32_output) {
        const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
        size_t pos = 0;
        char32_t *start{utf32_output};
        while (pos < len) {
            uint16_t word = !match_system(big_endian) ? gbswap_16(data[pos]) : data[pos];
            if ((word & 0xF800) != 0xD800) {
                // No surrogate pair, extend 16-bit word to 32-bit word
                *utf32_output++ = char32_t(word);
                pos++;
            } else {
                // must be a surrogate pair
                uint16_t diff = uint16_t(word - 0xD800);
                if (pos + 1 >= len) { return 0; } // minimal bound checking
                uint16_t next_word = !match_system(big_endian) ? gbswap_16(data[pos + 1]) : data[pos + 1];
                uint16_t diff2 = uint16_t(next_word - 0xDC00);
                uint32_t value = (diff << 10) + diff2 + 0x10000;
                *utf32_output++ = char32_t(value);
                pos += 2;
            }
        }
        return utf32_output - start;
    }

}  // namespace turbo::unicode::utf16_to_utf32

#endif  // TURBO_UNICODE_SCALAR_UTF16_CONVERT_H_