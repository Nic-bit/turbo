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
//

#ifndef  TURBO_VAR_INTERNAL_SERIES_H_
#define  TURBO_VAR_INTERNAL_SERIES_H_

#include <math.h>                       // round
#include <ostream>
#include <mutex>
#include <type_traits>
#include "turbo/container/batch.h"
#include "turbo/var/internal/call_op_returning_void.h"
#include "turbo/strings/str_split.h"
#include "turbo/platform/port.h"

namespace turbo ::var_internal {

    template<typename T, typename Op, typename Enabler = void>
    struct DivideOnAddition {
        static void inplace_divide(T & /*obj*/, const Op &, int /*number*/) {
            // do nothing
        }
    };

    template<typename T, typename Op>
    struct ProbablyAddtition {
        ProbablyAddtition(const Op &op) {
            T res(32);
            call_op_returning_void(op, res, T(64));
            _ok = (res == T(96));  // works for integral/floating point.
        }

        operator bool() const { return _ok; }

    private:
        bool _ok;
    };

    template<typename T, typename Op>
    struct DivideOnAddition<T, Op, typename std::enable_if<
            std::is_integral<T>::value>::type> {
        static void inplace_divide(T &obj, const Op &op, int number) {
            static ProbablyAddtition<T, Op> probably_add(op);
            if (probably_add) {
                obj = (T) round(obj / (double) number);
            }
        }
    };

    template<typename T, typename Op>
    struct DivideOnAddition<T, Op, typename std::enable_if<
            std::is_floating_point<T>::value>::type> {
        static void inplace_divide(T &obj, const Op &op, int number) {
            static ProbablyAddtition<T, Op> probably_add(op);
            if (probably_add) {
                obj /= number;
            }
        }
    };

    template<typename T, size_t N, typename Op>
    struct DivideOnAddition<Batch<T, N>, Op, typename std::enable_if<
            std::is_integral<T>::value>::type> {
        static void inplace_divide(Batch<T, N> &obj, const Op &op, int number) {
            static ProbablyAddtition<Batch<T, N>, Op> probably_add(op);
            if (probably_add) {
                for (size_t i = 0; i < N; ++i) {
                    obj[i] = (T) round(obj[i] / (double) number);
                }
            }
        }
    };

    template<typename T, size_t N, typename Op>
    struct DivideOnAddition<Batch<T, N>, Op, typename std::enable_if<
            std::is_floating_point<T>::value>::type> {
        static void inplace_divide(Batch<T, N> &obj, const Op &op, int number) {
            static ProbablyAddtition<Batch<T, N>, Op> probably_add(op);
            if (probably_add) {
                obj /= number;
            }
        }
    };

    template<typename T, typename Op>
    class SeriesBase {
    public:
        explicit SeriesBase(const Op &op)
                : _op(op), _nsecond(0), _nminute(0), _nhour(0), _nday(0) {
        }

        ~SeriesBase() {
        }

        void append(const T &value) {
            std::unique_lock l(_mutex);
            return append_second(value, _op);
        }

    private:
        void append_second(const T &value, const Op &op);

        void append_minute(const T &value, const Op &op);

        void append_hour(const T &value, const Op &op);

        void append_day(const T &value);

        struct Data {
        public:
            Data() {
                if (std::is_integral_v<T> ||
                    std::is_floating_point_v<T>) {
                    memset(static_cast<void *>(_array), 0, sizeof(_array));
                }
            }

            T &second(int index) { return _array[index]; }

            const T &second(int index) const { return _array[index]; }

            T &minute(int index) { return _array[60 + index]; }

            const T &minute(int index) const { return _array[60 + index]; }

            T &hour(int index) { return _array[120 + index]; }

            const T &hour(int index) const { return _array[120 + index]; }

            T &day(int index) { return _array[144 + index]; }

            const T &day(int index) const { return _array[144 + index]; }

        private:
            T _array[60 + 60 + 24 + 30];
        };

    protected:
        Op _op;
        mutable std::mutex _mutex;
        char _nsecond;
        char _nminute;
        char _nhour;
        char _nday;
        Data _data;
    };

    template<typename T, typename Op>
    void SeriesBase<T, Op>::append_second(const T &value, const Op &op) {
        _data.second(_nsecond) = value;
        ++_nsecond;
        if (_nsecond >= 60) {
            _nsecond = 0;
            T tmp = _data.second(0);
            for (int i = 1; i < 60; ++i) {
                call_op_returning_void(op, tmp, _data.second(i));
            }
            DivideOnAddition<T, Op>::inplace_divide(tmp, op, 60);
            append_minute(tmp, op);
        }
    }

    template<typename T, typename Op>
    void SeriesBase<T, Op>::append_minute(const T &value, const Op &op) {
        _data.minute(_nminute) = value;
        ++_nminute;
        if (_nminute >= 60) {
            _nminute = 0;
            T tmp = _data.minute(0);
            for (int i = 1; i < 60; ++i) {
                call_op_returning_void(op, tmp, _data.minute(i));
            }
            DivideOnAddition<T, Op>::inplace_divide(tmp, op, 60);
            append_hour(tmp, op);
        }
    }

    template<typename T, typename Op>
    void SeriesBase<T, Op>::append_hour(const T &value, const Op &op) {
        _data.hour(_nhour) = value;
        ++_nhour;
        if (_nhour >= 24) {
            _nhour = 0;
            T tmp = _data.hour(0);
            for (int i = 1; i < 24; ++i) {
                call_op_returning_void(op, tmp, _data.hour(i));
            }
            DivideOnAddition<T, Op>::inplace_divide(tmp, op, 24);
            append_day(tmp);
        }
    }

    template<typename T, typename Op>
    void SeriesBase<T, Op>::append_day(const T &value) {
        _data.day(_nday) = value;
        ++_nday;
        if (_nday >= 30) {
            _nday = 0;
        }
    }

    template<typename T, typename Op>
    class Series : public SeriesBase<T, Op> {
        typedef SeriesBase<T, Op> Base;
    public:
        explicit Series(const Op &op) : Base(op) {}

        void describe(std::ostream &os, const std::string *vector_names) const;
    };

    template<typename T, size_t N, typename Op>
    class Series<Batch<T, N>, Op> : public SeriesBase<Batch<T, N>, Op> {
        typedef SeriesBase<Batch<T, N>, Op> Base;
    public:
        explicit Series(const Op &op) : Base(op) {}

        void describe(std::ostream &os, const std::string *vector_names) const;
    };

    template<typename T, typename Op>
    void Series<T, Op>::describe(std::ostream &os,
                                 const std::string *vector_names) const {
        TURBO_ASSERT(vector_names == nullptr);
        this->_mutex.lock();
        const int second_begin = this->_nsecond;
        const int minute_begin = this->_nminute;
        const int hour_begin = this->_nhour;
        const int day_begin = this->_nday;
        // NOTE: we don't save _data which may be inconsistent sometimes, but
        // this output is generally for "peeking the trend" and does not need
        // to exactly accurate.
        this->_mutex.unlock();
        int c = 0;
        os << "{\"label\":\"trend\",\"data\":[";
        for (int i = 0; i < 30; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.day((i + day_begin) % 30) << ']';
        }
        for (int i = 0; i < 24; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.hour((i + hour_begin) % 24) << ']';
        }
        for (int i = 0; i < 60; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.minute((i + minute_begin) % 60) << ']';
        }
        for (int i = 0; i < 60; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.second((i + second_begin) % 60) << ']';
        }
        os << "]}";
    }

    template<typename T, size_t N, typename Op>
    void Series<Batch<T, N>, Op>::describe(std::ostream &os,
                                           const std::string *vector_names) const {
        this->_mutex.lock();
        const int second_begin = this->_nsecond;
        const int minute_begin = this->_nminute;
        const int hour_begin = this->_nhour;
        const int day_begin = this->_nday;
        // NOTE: we don't save _data which may be inconsistent sometimes, but
        // this output is generally for "peeking the trend" and does not need
        // to exactly accurate.
        this->_mutex.unlock();

        std::vector<std::string_view> fields = turbo::str_split(vector_names ? vector_names->c_str() : "", ',');
        auto it = fields.begin();
        os << '[';
        for (size_t j = 0; j < N; ++j) {
            if (j) {
                os << ',';
            }
            int c = 0;
            os << "{\"label\":\"";
            if (it != fields.end()) {
                os << *it;
                ++it;
            } else {
                os << "Vector[" << j << ']';
            }
            os << "\",\"data\":[";
            for (int i = 0; i < 30; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.day((i + day_begin) % 30)[j] << ']';
            }
            for (int i = 0; i < 24; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.hour((i + hour_begin) % 24)[j] << ']';
            }
            for (int i = 0; i < 60; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.minute((i + minute_begin) % 60)[j] << ']';
            }
            for (int i = 0; i < 60; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.second((i + second_begin) % 60)[j] << ']';
            }
            os << "]}";
        }
        os << ']';
    }

}  // namespace turbo::var_internal

#endif  // TURBO_VAR_INTERNAL_SERIES_H_
