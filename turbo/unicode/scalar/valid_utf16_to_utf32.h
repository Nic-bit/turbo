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

#ifndef TURBO_UNICODE_SCALAR_VALID_UTF16_TO_UTF32_H_
#define TURBO_UNICODE_SCALAR_VALID_UTF16_TO_UTF32_H_

namespace turbo {
namespace scalar {
namespace {
namespace utf16_to_utf32 {

template <EndianNess big_endian>
inline size_t convert_valid(const char16_t* buf, size_t len, char32_t* utf32_output) {
 const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
  size_t pos = 0;
  char32_t* start{utf32_output};
  while (pos < len) {
    uint16_t word = !match_system(big_endian) ? utf16::swap_bytes(data[pos]) : data[pos];
    if((word &0xF800 ) != 0xD800) {
      // No surrogate pair, extend 16-bit word to 32-bit word
      *utf32_output++ = char32_t(word);
      pos++;
    } else {
      // must be a surrogate pair
      uint16_t diff = uint16_t(word - 0xD800);
      if(pos + 1 >= len) { return 0; } // minimal bound checking
      uint16_t next_word = !match_system(big_endian) ? utf16::swap_bytes(data[pos + 1]) : data[pos + 1];
      uint16_t diff2 = uint16_t(next_word - 0xDC00);
      uint32_t value = (diff << 10) + diff2 + 0x10000;
      *utf32_output++ = char32_t(value);
      pos += 2;
    }
  }
  return utf32_output - start;
}

} // utf16_to_utf32 namespace
} // unnamed namespace
} // namespace scalar
} // namespace turbo

#endif  // TURBO_UNICODE_SCALAR_VALID_UTF16_TO_UTF32_H_
