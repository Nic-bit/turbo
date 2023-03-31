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

#include "turbo/unicode/fallback/begin.h"

#include "turbo/unicode/scalar/valid_utf8_to_utf16.h"
#include "turbo/unicode/scalar/utf8_to_utf16.h"

#include "turbo/unicode/scalar/valid_utf8_to_utf32.h"
#include "turbo/unicode/scalar/utf8_to_utf32.h"

#include "turbo/unicode/scalar/valid_utf16_to_utf8.h"
#include "turbo/unicode/scalar/utf16_to_utf8.h"

#include "turbo/unicode/scalar/valid_utf16_to_utf32.h"
#include "turbo/unicode/scalar/utf16_to_utf32.h"

#include "turbo/unicode/scalar/valid_utf32_to_utf8.h"
#include "turbo/unicode/scalar/utf32_to_utf8.h"

#include "turbo/unicode/scalar/valid_utf32_to_utf16.h"
#include "turbo/unicode/scalar/utf32_to_utf16.h"

#include "turbo/unicode/scalar/ascii.h"
#include "turbo/unicode/scalar/utf8.h"
#include "turbo/unicode/scalar/utf16.h"

namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {

TURBO_MUST_USE_RESULT int implementation::detect_encodings(const char * input, size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = turbo::BOM::check_bom(input, length);
  if(bom_encoding != encoding_type::unspecified) { return bom_encoding; }
  int out = 0;
  if(validate_utf8(input, length)) { out |= encoding_type::UTF8; }
  if((length % 2) == 0) {
    if(validate_utf16le(reinterpret_cast<const char16_t*>(input), length/2)) { out |= encoding_type::UTF16_LE; }
  }
  if((length % 4) == 0) {
    if(validate_utf32(reinterpret_cast<const char32_t*>(input), length/4)) { out |= encoding_type::UTF32_LE; }
  }

  return out;
}

TURBO_MUST_USE_RESULT bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
    return scalar::utf8::validate(buf, len);
}

TURBO_MUST_USE_RESULT result implementation::validate_utf8_with_errors(const char *buf, size_t len) const noexcept {
    return scalar::utf8::validate_with_errors(buf, len);
}

TURBO_MUST_USE_RESULT bool implementation::validate_ascii(const char *buf, size_t len) const noexcept {
    return scalar::ascii::validate(buf, len);
}

TURBO_MUST_USE_RESULT result implementation::validate_ascii_with_errors(const char *buf, size_t len) const noexcept {
    return scalar::ascii::validate_with_errors(buf, len);
}

TURBO_MUST_USE_RESULT bool implementation::validate_utf16le(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate<endianness::LITTLE>(buf, len);
}

TURBO_MUST_USE_RESULT bool implementation::validate_utf16be(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate<endianness::BIG>(buf, len);
}

TURBO_MUST_USE_RESULT result implementation::validate_utf16le_with_errors(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate_with_errors<endianness::LITTLE>(buf, len);
}

TURBO_MUST_USE_RESULT result implementation::validate_utf16be_with_errors(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate_with_errors<endianness::BIG>(buf, len);
}

TURBO_MUST_USE_RESULT bool implementation::validate_utf32(const char32_t *buf, size_t len) const noexcept {
    return scalar::utf32::validate(buf, len);
}

TURBO_MUST_USE_RESULT result implementation::validate_utf32_with_errors(const char32_t *buf, size_t len) const noexcept {
    return scalar::utf32::validate_with_errors(buf, len);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf8_to_utf16le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert<endianness::LITTLE>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf8_to_utf16be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert<endianness::BIG>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf8_to_utf16le_with_errors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert_with_errors<endianness::LITTLE>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf8_to_utf16be_with_errors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert_with_errors<endianness::BIG>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf8_to_utf16le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert_valid<endianness::LITTLE>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf8_to_utf16be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert_valid<endianness::BIG>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf8_to_utf32(const char* buf, size_t len, char32_t* utf32_output) const noexcept {
   return scalar::utf8_to_utf32::convert(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf8_to_utf32_with_errors(const char* buf, size_t len, char32_t* utf32_output) const noexcept {
   return scalar::utf8_to_utf32::convert_with_errors(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf8_to_utf32(const char* input, size_t size,
    char32_t* utf32_output) const noexcept {
  return scalar::utf8_to_utf32::convert_valid(input, size,  utf32_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf16le_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert<endianness::LITTLE>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf16be_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert<endianness::BIG>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf16le_to_utf8_with_errors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_with_errors<endianness::LITTLE>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf16be_to_utf8_with_errors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_with_errors<endianness::BIG>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf16le_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_valid<endianness::LITTLE>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf16be_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_valid<endianness::BIG>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf32_to_utf8_with_errors(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert_with_errors(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert_valid(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf32_to_utf16le(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert<endianness::LITTLE>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf32_to_utf16be(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert<endianness::BIG>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf32_to_utf16le_with_errors(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<endianness::LITTLE>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf32_to_utf16be_with_errors(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<endianness::BIG>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf32_to_utf16le(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_valid<endianness::LITTLE>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf32_to_utf16be(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_valid<endianness::BIG>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf16le_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert<endianness::LITTLE>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_utf16be_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert<endianness::BIG>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf16le_to_utf32_with_errors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_with_errors<endianness::LITTLE>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT result implementation::convert_utf16be_to_utf32_with_errors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_with_errors<endianness::BIG>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf16le_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_valid<endianness::LITTLE>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t implementation::convert_valid_utf16be_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_valid<endianness::BIG>(buf, len, utf32_output);
}

void implementation::change_endianness_utf16(const char16_t * input, size_t length, char16_t * output) const noexcept {
  scalar::utf16::change_endianness_utf16(input, length, output);
}

TURBO_MUST_USE_RESULT size_t implementation::count_utf16le(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::count_code_points<endianness::LITTLE>(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::count_utf16be(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::count_code_points<endianness::BIG>(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::count_utf8(const char * input, size_t length) const noexcept {
  return scalar::utf8::count_code_points(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::utf8_length_from_utf16le(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::utf8_length_from_utf16<endianness::LITTLE>(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::utf8_length_from_utf16be(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::utf8_length_from_utf16<endianness::BIG>(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::utf32_length_from_utf16le(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::utf32_length_from_utf16<endianness::LITTLE>(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::utf32_length_from_utf16be(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::utf32_length_from_utf16<endianness::BIG>(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::utf16_length_from_utf8(const char * input, size_t length) const noexcept {
  return scalar::utf8::utf16_length_from_utf8(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::utf8_length_from_utf32(const char32_t * input, size_t length) const noexcept {
  return scalar::utf32::utf8_length_from_utf32(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::utf16_length_from_utf32(const char32_t * input, size_t length) const noexcept {
  return scalar::utf32::utf16_length_from_utf32(input, length);
}

TURBO_MUST_USE_RESULT size_t implementation::utf32_length_from_utf8(const char * input, size_t length) const noexcept {
  return scalar::utf8::count_code_points(input, length);
}

} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo

#include "turbo/unicode/fallback/end.h"