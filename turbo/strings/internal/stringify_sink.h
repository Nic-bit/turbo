// Copyright 2022 The Turbo Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TURBO_STRINGS_INTERNAL_STRINGIFY_SINK_H_
#define TURBO_STRINGS_INTERNAL_STRINGIFY_SINK_H_

#include <string>
#include <type_traits>
#include <utility>

#include "turbo/strings/string_piece.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace strings_internal {
class StringifySink {
 public:
  void Append(size_t count, char ch);

  void Append(string_piece v);

  // Support `turbo::Format(&sink, format, args...)`.
  friend void TurboFormatFlush(StringifySink* sink, turbo::string_piece v) {
    sink->Append(v);
  }

 private:
  template <typename T>
  friend string_piece ExtractStringification(StringifySink& sink, const T& v);

  std::string buffer_;
};

template <typename T>
string_piece ExtractStringification(StringifySink& sink, const T& v) {
  TurboStringify(sink, v);
  return sink.buffer_;
}

}  // namespace strings_internal

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_STRINGIFY_SINK_H_
