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

#include "turbo/unicode/utf.h"

#include <array>
#include <algorithm>

#include <tests/unicode/helpers/random_utf8.h>
#include <tests/unicode/helpers/test.h>
#include <fstream>
#include <iostream>
#include <memory>

constexpr size_t num_trials = 1000;

TEST(no_error) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf8 generator{seed, 1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    const auto utf8{generator.generate(512, seed)};
    turbo::result res = implementation.ValidateUtf8WithErrors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
    ASSERT_EQUAL(res.error, turbo::error_code::SUCCESS);
    ASSERT_EQUAL(res.count, utf8.size());
  }
}


TEST(header_bits_error) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf8 generator{seed, 1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512, seed)};

    for (int i = 0; i < 512; i++) {
      if((utf8[i] & 0b11000000) != 0b10000000) {  // Only process leading bytes
        const unsigned char old = utf8[i];
        utf8[i] = uint8_t(0b11111000);
        turbo::result res = implementation.ValidateUtf8WithErrors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        ASSERT_EQUAL(res.error, turbo::error_code::HEADER_BITS);
        ASSERT_EQUAL(res.count, i);
        utf8[i] = old;
      }
    }
  }
}

TEST(too_short_error) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf8 generator{seed, 1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512, seed)};
    int leading_byte_pos = 0;
    for (int i = 0; i < 512; i++) {
      if((utf8[i] & 0b11000000) == 0b10000000) {  // Only process continuation bytes by making them leading bytes
        const unsigned char old = utf8[i];
        utf8[i] = uint8_t(0b11100000);
        turbo::result res = implementation.ValidateUtf8WithErrors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        ASSERT_EQUAL(res.error, turbo::error_code::TOO_SHORT);
        ASSERT_EQUAL(res.count, leading_byte_pos);
        utf8[i] = old;
      } else {
        leading_byte_pos = i;
      }
    }
  }
}

TEST(too_long_error) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf8 generator{seed, 1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512, seed)};
    for (int i = 1; i < 512; i++) {
      if(((utf8[i] & 0b11000000) != 0b10000000)) {  // Only process leading bytes by making them continuation bytes
        const unsigned char old = utf8[i];
        utf8[i] = uint8_t(0b10000000);
        turbo::result res = implementation.ValidateUtf8WithErrors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        ASSERT_EQUAL(res.error, turbo::error_code::TOO_LONG);
        ASSERT_EQUAL(res.count, i);
        utf8[i] = old;
      }
    }
  }
}

TEST(overlong_error) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf8 generator{seed, 1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512, seed)};
    for (int i = 1; i < 512; i++) {
      if(utf8[i] >= 0b11000000) { // Only non-ASCII leading bytes can be overlong
        const unsigned char old = utf8[i];
        const unsigned char second_old = utf8[i+1];
        if ((old & 0b11100000) == 0b11000000) { // two-bytes case, change to a value less or equal than 0x7f
          utf8[i] = 0b11000000;
        } else if ((old & 0b11110000) == 0b11100000) {  // three-bytes case, change to a value less or equal than 0x7ff
          utf8[i] = 0b11100000;
          utf8[i+1] = utf8[i+1] & 0b11011111;
        } else {  // four-bytes case, change to a value less or equal than 0xffff
          utf8[i] = 0b11110000;
          utf8[i+1] = utf8[i+1] & 0b11001111;
        }
        turbo::result res = implementation.ValidateUtf8WithErrors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        ASSERT_EQUAL(res.error, turbo::error_code::OVERLONG);
        ASSERT_EQUAL(res.count, i);
        utf8[i] = old;
        utf8[i+1] = second_old;
      }
    }
  }
}

TEST(too_large_error) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf8 generator{seed, 1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512, seed)};
    for (int i = 1; i < 512; i++) {
      if((utf8[i] & 0b11111000) == 0b11110000) { // Can only have too large error in 4-bytes case
        utf8[i] += ((utf8[i] & 0b100) == 0b100) ? 0b10 : 0b100;   // Make sure we get too large error and not header bits error
        turbo::result res = implementation.ValidateUtf8WithErrors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        ASSERT_EQUAL(res.error, turbo::error_code::TOO_LARGE);
        ASSERT_EQUAL(res.count, i);
        utf8[i] -= 0b100;
      }
    }
  }
}

TEST(surrogate_error) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf8 generator{seed, 1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512, seed)};
    for (int i = 1; i < 512; i++) {
      if((utf8[i] & 0b11110000) == 0b11100000) { // Can only have surrogate error in 3-bytes case
        const unsigned char old = utf8[i];
        const unsigned char second_old = utf8[i+1];
        utf8[i] = 0b11101101;                 // Leading byte is always the same
        for (int s = 0x8; s < 0xf; s++) {  // Modify second byte to create a surrogate codepoint
          utf8[i+1] = (utf8[i+1] & 0b11000011) | (s << 2);
          turbo::result res = implementation.ValidateUtf8WithErrors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
          ASSERT_EQUAL(res.error, turbo::error_code::SURROGATE);
          ASSERT_EQUAL(res.count, i);
        }
        utf8[i] = old;
        utf8[i+1] = second_old;
      }
    }
  }
}


int main(int argc, char* argv[]) {
  return turbo::test::main(argc, argv);
}