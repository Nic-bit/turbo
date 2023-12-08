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
// Created by jeff on 23-12-7.
//

#ifndef TURBO_UTF_SCALAR_VALIDATE_H_
#define TURBO_UTF_SCALAR_VALIDATE_H_

#include "turbo/utf/avx2/utf_converter_fwd.h"

namespace turbo {

    bool UtfConverter<UtfAvx2Engine>::validate_utf8(const char *buf, size_t len) const noexcept {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        uint64_t pos = 0;
        uint32_t code_point = 0;
        while (pos < len) {
            // check of the next 8 bytes are ascii.
            uint64_t next_pos = pos + 16;
            if (next_pos <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v1;
                std::memcpy(&v1, data + pos, sizeof(uint64_t));
                uint64_t v2;
                std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
                uint64_t v{v1 | v2};
                if ((v & 0x8080808080808080) == 0) {
                    pos = next_pos;
                    continue;
                }
            }
            unsigned char byte = data[pos];

            while (byte < 0b10000000) {
                if (++pos == len) { return true; }
                byte = data[pos];
            }

            if ((byte & 0b11100000) == 0b11000000) {
                next_pos = pos + 2;
                if (next_pos > len) { return false; }
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
                // range check
                code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
                if ((code_point < 0x80) || (0x7ff < code_point)) { return false; }
            } else if ((byte & 0b11110000) == 0b11100000) {
                next_pos = pos + 3;
                if (next_pos > len) { return false; }
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
                // range check
                code_point = (byte & 0b00001111) << 12 |
                             (data[pos + 1] & 0b00111111) << 6 |
                             (data[pos + 2] & 0b00111111);
                if ((code_point < 0x800) || (0xffff < code_point) ||
                    (0xd7ff < code_point && code_point < 0xe000)) {
                    return false;
                }
            } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
                next_pos = pos + 4;
                if (next_pos > len) { return false; }
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
                if ((data[pos + 3] & 0b11000000) != 0b10000000) { return false; }
                // range check
                code_point =
                        (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
                        (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
                if (code_point <= 0xffff || 0x10ffff < code_point) { return false; }
            } else {
                // we may have a continuation
                return false;
            }
            pos = next_pos;
        }
        return true;
    }
}  // namespace turbo

#endif  // TURBO_UTF_SCALAR_VALIDATE_H_
