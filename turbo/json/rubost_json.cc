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

#include "turbo/json/robust_json.h"
#include <fstream>
#include <cerrno>
#include "turbo/strings/numbers.h"
#include "turbo/json/ostreamwrapper.h"
#include "turbo/json/writer.h"
#include "turbo/files/filesystem.h"
#include "turbo/files/sequential_read_file.h"

#define ROBUST_JSON_DEFINE_BOTH_CAST(T, TPL_FUNC)                  \
  template <>                                                      \
  template <>                                                      \
  T robust_json<rapidjson::Value>::cast<T>() const noexcept {       \
    return TPL_FUNC;                                               \
  }                                                                \
  template <>                                                      \
  template <>                                                      \
  T robust_json<const rapidjson::Value>::cast<T>() const noexcept { \
    return TPL_FUNC;                                               \
  }

#define ROBUST_JSON_DEFINE_BOTH_AS(T, TPL_FUNC)                                 \
  template <>                                                                   \
  template <>                                                                   \
  turbo::optional<T> robust_json<rapidjson::Value>::as<T>() const noexcept {       \
    return TPL_FUNC;                                                            \
  }                                                                             \
  template <>                                                                   \
  template <>                                                                   \
  turbo::optional<T> robust_json<const rapidjson::Value>::as<T>() const noexcept { \
    return TPL_FUNC;                                                            \
  }


RAPIDJSON_NAMESPACE_BEGIN

    turbo::Status robust_parse(const std::string &file_path, rapidjson::Document *doc) {
        std::error_code ec;
        auto r = turbo::filesystem::exists(file_path, ec);
        if (ec) {
            return turbo::ErrnoToStatus(ec.value(), "check file exists error");
        }
        if(!r) {
            return turbo::NotFoundError("not exists file");
        }

        turbo::SequentialReadFile file;
        auto rs = file.open(file_path);
        if(!rs.ok()) {
            return rs;
        }
        std::string content;
        rs = file.read(&content);
        if(!rs.ok()) {
            return rs;
        }
        if (!static_cast<rapidjson::ParseResult>(doc->Parse(content.c_str()))) {
            return turbo::DataLossError("json parse error");
        }
        return turbo::OkStatus();
    }


    turbo::Status robust_write(const rapidjson::Document &d, const std::string &file_path) {
        std::ofstream ofs(file_path);
        if (!ofs) {
            return turbo::ErrnoToStatus(errno, "open file error");
        }
        rapidjson::OStreamWrapper osw(ofs);
        rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
        if (!d.Accept(writer)) {
            return  turbo::ErrnoToStatus(errno, "write file error");
        }
        return turbo::OkStatus();
    }


    template<typename OptionalRefRapidJsonValue>
    turbo::string_piece cast_string_view(OptionalRefRapidJsonValue v_) {
        if (v_.has_value() && v_.value().get().IsString()) {
            return turbo::string_piece{v_.value().get().GetString(), v_.value().get().GetStringLength()};
        }
        return turbo::string_piece{};
    }

    ROBUST_JSON_DEFINE_BOTH_CAST(turbo::string_piece, cast_string_view(v_));

    template<typename OptionalRefRapidJsonValue>
    bool cast_bool(OptionalRefRapidJsonValue v_) {
        if (!v_.has_value()) {
            return false;
        }
        auto &v = v_.value().get();
        if (v.IsBool()) {
            return v.GetBool();
        }
        if (v.IsNull()) {
            return false;
        }
        if (v.IsString()) {
            return !(v.GetStringLength() == 0 || (v.GetStringLength() == 1 && *v.GetString() == '0'));
        }
        if (v.IsInt64()) {
            return v.GetInt64() != 0;
        }
        return true;
    }

    ROBUST_JSON_DEFINE_BOTH_CAST(bool, cast_bool(v_));

    const rapidjson::Value kEmptyConstArray{rapidjson::kArrayType};

    template<typename OptionalRefRapidJsonValue>
    rapidjson::Value::ConstArray cast_const_array(OptionalRefRapidJsonValue v_) {
        if (v_.has_value() && v_.value().get().IsArray()) {
            return static_cast<const rapidjson::Value &>(v_.value().get()).GetArray();
        }
        return kEmptyConstArray.GetArray();
    }

    ROBUST_JSON_DEFINE_BOTH_CAST(rapidjson::Value::ConstArray, cast_const_array(v_));

    template<typename OptionalRefRapidJsonValue>
    turbo::optional<rapidjson::Value::ConstObject> as_const_object(OptionalRefRapidJsonValue v_) {
        if (v_.has_value() && v_.value().get().IsObject()) {
            return turbo::make_optional(static_cast<const rapidjson::Value &>(v_.value().get()).GetObject());
        }
        return turbo::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(rapidjson::Value::ConstObject, as_const_object(v_));

    template<>
    template<>
    turbo::optional<rapidjson::Value::Object>
    robust_json<rapidjson::Value>::as<rapidjson::Value::Object>() const noexcept {
        if (v_.has_value() && v_.value().get().IsObject()) {
            return turbo::make_optional(v_.value().get().GetObject());
        }
        return turbo::nullopt;
    }

    template<typename OptionalRefRapidJsonValue>
    turbo::optional<rapidjson::Value::ConstArray> as_const_array(OptionalRefRapidJsonValue v_) {
        if (v_.has_value() && v_.value().get().IsArray()) {
            return turbo::make_optional(static_cast<const rapidjson::Value &>(v_.value().get()).GetArray());
        }
        return turbo::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(rapidjson::Value::ConstArray, as_const_array(v_));

    template<>
    template<>
    turbo::optional<rapidjson::Value::Array> robust_json<rapidjson::Value>::as<rapidjson::Value::Array>() const noexcept {
        if (v_.has_value() && v_.value().get().IsArray()) {
            return turbo::make_optional(v_.value().get().GetArray());
        }
        return turbo::nullopt;
    }

    template<typename OptionalRefRapidJsonValue>
    turbo::optional<uint64_t> as_uint64(OptionalRefRapidJsonValue v_) {
        if (v_.has_value()) {
            auto &v = v_.value().get();
            if (v.IsUint64()) {
                // compatible with kNumberUintFlag/kNumberUint64Flag values
                return turbo::make_optional(v.GetUint64());
            }
            if (v.IsString()) {
                uint64_t n;
                if (!turbo::SimpleAtoi(turbo::string_piece{v.GetString(), v.GetStringLength()}, &n)) {
                    return turbo::nullopt;
                }
                return n;
            }
        }
        return turbo::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(uint64_t, as_uint64(v_));

    template<typename OptionalRefRapidJsonValue>
    turbo::optional<int64_t> as_int64(OptionalRefRapidJsonValue v_) {
        if (v_.has_value()) {
            auto &v = v_.value().get();
            if (v.IsInt64()) {
                // compatible with kNumberIntFlag/kNumberUintFlag/kNumberInt64Flag values
                return turbo::make_optional(v.GetInt64());
            }
            if (v.IsString()) {
                int64_t n;
                if (!turbo::SimpleAtoi(turbo::string_piece{v.GetString(), v.GetStringLength()}, &n)) {
                    return turbo::nullopt;
                }
                return n;
            }
        }
        return turbo::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(int64_t, as_int64(v_));

    template<typename OptionalRefRapidJsonValue>
    turbo::optional<double> as_double(OptionalRefRapidJsonValue v_) {
        if (v_.has_value()) {
            auto &v = v_.value().get();
            if (v.IsNumber()) {
                // compatible with any number values
                return turbo::make_optional(v.GetDouble());
            }
            if (v.IsString()) {
                double n;
                if (!turbo::SimpleAtod(turbo::string_piece{v.GetString(), v.GetStringLength()}, &n)) {
                    return turbo::nullopt;
                }
                return n;
            }
        }
        return turbo::nullopt;
    }

    ROBUST_JSON_DEFINE_BOTH_AS(double, as_double(v_));


RAPIDJSON_NAMESPACE_END