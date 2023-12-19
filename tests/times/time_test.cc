// Copyright 2020 The Turbo Authors.
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

#include "turbo/times/time.h"

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <chrono>  // NOLINT(build/c++11)
#include <cstring>
#include <ctime>
#include <iomanip>
#include <limits>
#include <string>

#include "turbo/testing/test.h"
#include "turbo/base/int128.h"
#include "turbo/times/clock.h"
#include "tests/times/test_util.h"

namespace {


// This helper is a macro so that failed expectations show up with the
// correct line numbers.
#define REQUIRE_CIVIL_INFO(ci, y, m, d, h, min, s, off, isdst)      \
  do {                                                             \
    REQUIRE_EQ(y, ci.cs.year());                                    \
    REQUIRE_EQ(m, ci.cs.month());                                   \
    REQUIRE_EQ(d, ci.cs.day());                                     \
    REQUIRE_EQ(h, ci.cs.hour());                                    \
    REQUIRE_EQ(min, ci.cs.minute());                                \
    REQUIRE_EQ(s, ci.cs.second());                                  \
    REQUIRE_EQ(off, ci.offset);                                     \
    REQUIRE_EQ(isdst, ci.is_dst);                                   \
  } while (0)

    TEST_CASE("Time, ConstExpr") {
        constexpr turbo::Time t0 = turbo::unix_epoch();
        static_assert(t0 == turbo::Time(), "unix_epoch");
        constexpr turbo::Time t1 = turbo::infinite_future();
        static_assert(t1 != turbo::Time(), "infinite_future");
        constexpr turbo::Time t2 = turbo::infinite_past();
        static_assert(t2 != turbo::Time(), "infinite_past");
        constexpr turbo::Time t3 = turbo::from_unix_nanos(0);
        static_assert(t3 == turbo::Time(), "from_unix_nanos");
        constexpr turbo::Time t4 = turbo::from_unix_micros(0);
        static_assert(t4 == turbo::Time(), "from_unix_micros");
        constexpr turbo::Time t5 = turbo::from_unix_millis(0);
        static_assert(t5 == turbo::Time(), "from_unix_millis");
        constexpr turbo::Time t6 = turbo::from_unix_seconds(0);
        static_assert(t6 == turbo::Time(), "from_unix_seconds");
        constexpr turbo::Time t7 = turbo::from_time_t(0);
        static_assert(t7 == turbo::Time(), "from_time_t");
    }

    TEST_CASE("Time, ValueSemantics") {
        turbo::Time a;      // Default construction
        turbo::Time b = a;  // Copy construction
        REQUIRE_EQ(a, b);
        turbo::Time c(a);  // Copy construction (again)
        REQUIRE_EQ(a, b);
        REQUIRE_EQ(a, c);
        REQUIRE_EQ(b, c);
        b = c;  // Assignment
        REQUIRE_EQ(a, b);
        REQUIRE_EQ(a, c);
        REQUIRE_EQ(b, c);
    }

    TEST_CASE("time, unix_epoch") {
        const auto ci = turbo::utc_time_zone().At(turbo::unix_epoch());
        REQUIRE_EQ(turbo::CivilSecond(1970, 1, 1, 0, 0, 0), ci.cs);
        REQUIRE_EQ(turbo::zero_duration(), ci.subsecond);
        REQUIRE_EQ(turbo::Weekday::thursday, turbo::get_weekday(ci.cs));
    }

    TEST_CASE("Time, Breakdown") {
        turbo::TimeZone tz = turbo::time_internal::load_time_zone("America/New_York");
        turbo::Time t = turbo::unix_epoch();

        // The Unix epoch as seen in NYC.
        auto ci = tz.At(t);
        REQUIRE_CIVIL_INFO(ci, 1969, 12, 31, 19, 0, 0, -18000, false);
        REQUIRE_EQ(turbo::zero_duration(), ci.subsecond);
        REQUIRE_EQ(turbo::Weekday::wednesday, turbo::get_weekday(ci.cs));

        // Just before the epoch.
        t -= turbo::nanoseconds(1);
        ci = tz.At(t);
        REQUIRE_CIVIL_INFO(ci, 1969, 12, 31, 18, 59, 59, -18000, false);
        REQUIRE_EQ(turbo::nanoseconds(999999999), ci.subsecond);
        REQUIRE_EQ(turbo::Weekday::wednesday, turbo::get_weekday(ci.cs));

        // Some time later.
        t += turbo::hours(24) * 2735;
        t += turbo::hours(18) + turbo::minutes(30) + turbo::seconds(15) +
             turbo::nanoseconds(9);
        ci = tz.At(t);
        REQUIRE_CIVIL_INFO(ci, 1977, 6, 28, 14, 30, 15, -14400, true);
        REQUIRE_EQ(8, ci.subsecond / turbo::nanoseconds(1));
        REQUIRE_EQ(turbo::Weekday::tuesday, turbo::get_weekday(ci.cs));
    }

    TEST_CASE("Time, AdditiveOperators") {
        const turbo::Duration d = turbo::nanoseconds(1);
        const turbo::Time t0;
        const turbo::Time t1 = t0 + d;

        REQUIRE_EQ(d, t1 - t0);
        REQUIRE_EQ(-d, t0 - t1);
        REQUIRE_EQ(t0, t1 - d);

        turbo::Time t(t0);
        REQUIRE_EQ(t0, t);
        t += d;
        REQUIRE_EQ(t0 + d, t);
        REQUIRE_EQ(d, t - t0);
        t -= d;
        REQUIRE_EQ(t0, t);

        // Tests overflow between subseconds and seconds.
        t = turbo::unix_epoch();
        t += turbo::milliseconds(500);
        REQUIRE_EQ(turbo::unix_epoch() + turbo::milliseconds(500), t);
        t += turbo::milliseconds(600);
        REQUIRE_EQ(turbo::unix_epoch() + turbo::milliseconds(1100), t);
        t -= turbo::milliseconds(600);
        REQUIRE_EQ(turbo::unix_epoch() + turbo::milliseconds(500), t);
        t -= turbo::milliseconds(500);
        REQUIRE_EQ(turbo::unix_epoch(), t);
    }

    TEST_CASE("Time, RelationalOperators") {
        constexpr turbo::Time t1 = turbo::from_unix_nanos(0);
        constexpr turbo::Time t2 = turbo::from_unix_nanos(1);
        constexpr turbo::Time t3 = turbo::from_unix_nanos(2);

        static_assert(turbo::Time() == t1, "");
        static_assert(t1 == t1, "");
        static_assert(t2 == t2, "");
        static_assert(t3 == t3, "");

        static_assert(t1 < t2, "");
        static_assert(t2 < t3, "");
        static_assert(t1 < t3, "");

        static_assert(t1 <= t1, "");
        static_assert(t1 <= t2, "");
        static_assert(t2 <= t2, "");
        static_assert(t2 <= t3, "");
        static_assert(t3 <= t3, "");
        static_assert(t1 <= t3, "");

        static_assert(t2 > t1, "");
        static_assert(t3 > t2, "");
        static_assert(t3 > t1, "");

        static_assert(t2 >= t2, "");
        static_assert(t2 >= t1, "");
        static_assert(t3 >= t3, "");
        static_assert(t3 >= t2, "");
        static_assert(t1 >= t1, "");
        static_assert(t3 >= t1, "");
    }

    TEST_CASE("Time, Infinity") {
        constexpr turbo::Time ifuture = turbo::infinite_future();
        constexpr turbo::Time ipast = turbo::infinite_past();

        static_assert(ifuture == ifuture, "");
        static_assert(ipast == ipast, "");
        static_assert(ipast < ifuture, "");
        static_assert(ifuture > ipast, "");

        // Arithmetic saturates
        REQUIRE_EQ(ifuture, ifuture + turbo::seconds(1));
        REQUIRE_EQ(ifuture, ifuture - turbo::seconds(1));
        REQUIRE_EQ(ipast, ipast + turbo::seconds(1));
        REQUIRE_EQ(ipast, ipast - turbo::seconds(1));

        REQUIRE_EQ(turbo::infinite_duration(), ifuture - ifuture);
        REQUIRE_EQ(turbo::infinite_duration(), ifuture - ipast);
        REQUIRE_EQ(-turbo::infinite_duration(), ipast - ifuture);
        REQUIRE_EQ(-turbo::infinite_duration(), ipast - ipast);

        constexpr turbo::Time t = turbo::unix_epoch();  // Any finite time.
        static_assert(t < ifuture, "");
        static_assert(t > ipast, "");

        REQUIRE_EQ(ifuture, t + turbo::infinite_duration());
        REQUIRE_EQ(ipast, t - turbo::infinite_duration());
    }

    TEST_CASE("Time, FloorConversion") {
#define TEST_FLOOR_CONVERSION(TO, FROM) \
  REQUIRE_EQ(1, TO(FROM(1001)));         \
  REQUIRE_EQ(1, TO(FROM(1000)));         \
  REQUIRE_EQ(0, TO(FROM(999)));          \
  REQUIRE_EQ(0, TO(FROM(1)));            \
  REQUIRE_EQ(0, TO(FROM(0)));            \
  REQUIRE_EQ(-1, TO(FROM(-1)));          \
  REQUIRE_EQ(-1, TO(FROM(-999)));        \
  REQUIRE_EQ(-1, TO(FROM(-1000)));       \
  REQUIRE_EQ(-2, TO(FROM(-1001)));

        TEST_FLOOR_CONVERSION(turbo::to_unix_micros, turbo::from_unix_nanos);
        TEST_FLOOR_CONVERSION(turbo::to_unix_millis, turbo::from_unix_micros);
        TEST_FLOOR_CONVERSION(turbo::to_unix_seconds, turbo::from_unix_millis);
        TEST_FLOOR_CONVERSION(turbo::to_time_t, turbo::from_unix_millis);

#undef TEST_FLOOR_CONVERSION

        // Tests to_unix_nanos.
        REQUIRE_EQ(1, turbo::to_unix_nanos(turbo::unix_epoch() + turbo::nanoseconds(3) / 2));
        REQUIRE_EQ(1, turbo::to_unix_nanos(turbo::unix_epoch() + turbo::nanoseconds(1)));
        REQUIRE_EQ(0, turbo::to_unix_nanos(turbo::unix_epoch() + turbo::nanoseconds(1) / 2));
        REQUIRE_EQ(0, turbo::to_unix_nanos(turbo::unix_epoch() + turbo::nanoseconds(0)));
        REQUIRE_EQ(-1,
                  turbo::to_unix_nanos(turbo::unix_epoch() - turbo::nanoseconds(1) / 2));
        REQUIRE_EQ(-1, turbo::to_unix_nanos(turbo::unix_epoch() - turbo::nanoseconds(1)));
        REQUIRE_EQ(-2,
                  turbo::to_unix_nanos(turbo::unix_epoch() - turbo::nanoseconds(3) / 2));

        // Tests to_universal, which uses a different epoch than the tests above.
        REQUIRE_EQ(1,
                  turbo::to_universal(turbo::universal_epoch() + turbo::nanoseconds(101)));
        REQUIRE_EQ(1,
                  turbo::to_universal(turbo::universal_epoch() + turbo::nanoseconds(100)));
        REQUIRE_EQ(0,
                  turbo::to_universal(turbo::universal_epoch() + turbo::nanoseconds(99)));
        REQUIRE_EQ(0,
                  turbo::to_universal(turbo::universal_epoch() + turbo::nanoseconds(1)));
        REQUIRE_EQ(0,
                  turbo::to_universal(turbo::universal_epoch() + turbo::nanoseconds(0)));
        REQUIRE_EQ(-1,
                  turbo::to_universal(turbo::universal_epoch() + turbo::nanoseconds(-1)));
        REQUIRE_EQ(-1,
                  turbo::to_universal(turbo::universal_epoch() + turbo::nanoseconds(-99)));
        REQUIRE_EQ(
                -1, turbo::to_universal(turbo::universal_epoch() + turbo::nanoseconds(-100)));
        REQUIRE_EQ(
                -2, turbo::to_universal(turbo::universal_epoch() + turbo::nanoseconds(-101)));

        const struct {
            timespec ts;
            turbo::Time t;
        } from_ts[] = {
                {{1,  1},         turbo::from_unix_seconds(1) + turbo::nanoseconds(1)},
                {{1,  0},         turbo::from_unix_seconds(1) + turbo::nanoseconds(0)},
                {{0,  0},         turbo::from_unix_seconds(0) + turbo::nanoseconds(0)},
                {{0,  -1},        turbo::from_unix_seconds(0) - turbo::nanoseconds(1)},
                {{-1, 999999999}, turbo::from_unix_seconds(0) - turbo::nanoseconds(1)},
                {{-1, 1},         turbo::from_unix_seconds(-1) + turbo::nanoseconds(1)},
                {{-1, 0},         turbo::from_unix_seconds(-1) + turbo::nanoseconds(0)},
                {{-1, -1},        turbo::from_unix_seconds(-1) - turbo::nanoseconds(1)},
                {{-2, 999999999}, turbo::from_unix_seconds(-1) - turbo::nanoseconds(1)},
        };
        for (const auto &test: from_ts) {
            REQUIRE_EQ(test.t, turbo::time_from_timespec(test.ts));
        }

        const struct {
            timeval tv;
            turbo::Time t;
        } from_tv[] = {
                {{1,  1},      turbo::from_unix_seconds(1) + turbo::microseconds(1)},
                {{1,  0},      turbo::from_unix_seconds(1) + turbo::microseconds(0)},
                {{0,  0},      turbo::from_unix_seconds(0) + turbo::microseconds(0)},
                {{0,  -1},     turbo::from_unix_seconds(0) - turbo::microseconds(1)},
                {{-1, 999999}, turbo::from_unix_seconds(0) - turbo::microseconds(1)},
                {{-1, 1},      turbo::from_unix_seconds(-1) + turbo::microseconds(1)},
                {{-1, 0},      turbo::from_unix_seconds(-1) + turbo::microseconds(0)},
                {{-1, -1},     turbo::from_unix_seconds(-1) - turbo::microseconds(1)},
                {{-2, 999999}, turbo::from_unix_seconds(-1) - turbo::microseconds(1)},
        };
        for (const auto &test: from_tv) {
            REQUIRE_EQ(test.t, turbo::time_from_timeval(test.tv));
        }

        // Tests flooring near negative infinity.
        const int64_t min_plus_1 = std::numeric_limits<int64_t>::min() + 1;
        REQUIRE_EQ(min_plus_1, turbo::to_unix_seconds(turbo::from_unix_seconds(min_plus_1)));
        REQUIRE_EQ(std::numeric_limits<int64_t>::min(),
                  turbo::to_unix_seconds(turbo::from_unix_seconds(min_plus_1) -
                                       turbo::nanoseconds(1) / 2));

        // Tests flooring near positive infinity.
        REQUIRE_EQ(std::numeric_limits<int64_t>::max(),
                  turbo::to_unix_seconds(
                          turbo::from_unix_seconds(std::numeric_limits<int64_t>::max()) +
                          turbo::nanoseconds(1) / 2));
        REQUIRE_EQ(std::numeric_limits<int64_t>::max(),
                  turbo::to_unix_seconds(
                          turbo::from_unix_seconds(std::numeric_limits<int64_t>::max())));
        REQUIRE_EQ(std::numeric_limits<int64_t>::max() - 1,
                  turbo::to_unix_seconds(
                          turbo::from_unix_seconds(std::numeric_limits<int64_t>::max()) -
                          turbo::nanoseconds(1) / 2));
    }

    template<typename Duration>
    std::chrono::system_clock::time_point MakeChronoUnixTime(const Duration &d) {
        return std::chrono::system_clock::from_time_t(0) + d;
    }

    TEST_CASE("time, from_chrono") {
        REQUIRE_EQ(turbo::from_time_t(-1),
                  turbo::from_chrono(std::chrono::system_clock::from_time_t(-1)));
        REQUIRE_EQ(turbo::from_time_t(0),
                  turbo::from_chrono(std::chrono::system_clock::from_time_t(0)));
        REQUIRE_EQ(turbo::from_time_t(1),
                  turbo::from_chrono(std::chrono::system_clock::from_time_t(1)));

        REQUIRE_EQ(
                turbo::from_unix_millis(-1),
                turbo::from_chrono(MakeChronoUnixTime(std::chrono::milliseconds(-1))));
        REQUIRE_EQ(turbo::from_unix_millis(0),
                  turbo::from_chrono(MakeChronoUnixTime(std::chrono::milliseconds(0))));
        REQUIRE_EQ(turbo::from_unix_millis(1),
                  turbo::from_chrono(MakeChronoUnixTime(std::chrono::milliseconds(1))));

        // Chrono doesn't define exactly its range and precision (neither does
        // turbo::Time), so let's simply test +/- ~100 years to make sure things work.
        const auto century_sec = 60 * 60 * 24 * 365 * int64_t{100};
        const auto century = std::chrono::seconds(century_sec);
        const auto chrono_future = MakeChronoUnixTime(century);
        const auto chrono_past = MakeChronoUnixTime(-century);
        REQUIRE_EQ(turbo::from_unix_seconds(century_sec),
                  turbo::from_chrono(chrono_future));
        REQUIRE_EQ(turbo::from_unix_seconds(-century_sec), turbo::from_chrono(chrono_past));

        // Roundtrip them both back to chrono.
        REQUIRE_EQ(chrono_future,
                  turbo::to_chrono_time(turbo::from_unix_seconds(century_sec)));
        REQUIRE_EQ(chrono_past,
                  turbo::to_chrono_time(turbo::from_unix_seconds(-century_sec)));
    }

    TEST_CASE("time, to_chrono_time") {

        REQUIRE_EQ(std::chrono::system_clock::from_time_t(-1),
                  turbo::to_chrono_time(turbo::from_time_t(-1)));
        REQUIRE_EQ(std::chrono::system_clock::from_time_t(0),
                  turbo::to_chrono_time(turbo::from_time_t(0)));
        REQUIRE_EQ(std::chrono::system_clock::from_time_t(1),
                  turbo::to_chrono_time(turbo::from_time_t(1)));

        REQUIRE_EQ(MakeChronoUnixTime(std::chrono::milliseconds(-1)),
                  turbo::to_chrono_time(turbo::from_unix_millis(-1)));
        REQUIRE_EQ(MakeChronoUnixTime(std::chrono::milliseconds(0)),
                  turbo::to_chrono_time(turbo::from_unix_millis(0)));
        REQUIRE_EQ(MakeChronoUnixTime(std::chrono::milliseconds(1)),
                  turbo::to_chrono_time(turbo::from_unix_millis(1)));

        // Time before the Unix epoch should floor, not trunc.
        const auto tick = turbo::nanoseconds(1) / 4;
        REQUIRE_EQ(std::chrono::system_clock::from_time_t(0) -
                  std::chrono::system_clock::duration(1),
                  turbo::to_chrono_time(turbo::unix_epoch() - tick));
    }

// Check that turbo::int128 works as a std::chrono::duration representation.
    TEST_CASE("Time, Chrono128") {
        // Define a std::chrono::time_point type whose time[sic]_since_epoch() is
        // a signed 128-bit count of attoseconds. This has a range and resolution
        // (currently) beyond those of turbo::Time, and undoubtedly also beyond those
        // of std::chrono::system_clock::time_point.
        //
        // Note: The to/from-chrono support should probably be updated to handle
        // such wide representations.
        using Timestamp =
                std::chrono::time_point<std::chrono::system_clock,
                        std::chrono::duration<turbo::int128, std::atto>>;

        // Expect that we can round-trip the std::chrono::system_clock::time_point
        // extremes through both turbo::Time and Timestamp, and that Timestamp can
        // handle the (current) turbo::Time extremes.
        //
        // Note: We should use std::chrono::floor() instead of time_point_cast(),
        // but floor() is only available since c++17.
        for (const auto tp: {std::chrono::system_clock::time_point::min(),
                             std::chrono::system_clock::time_point::max()}) {
            REQUIRE_EQ(tp, turbo::to_chrono_time(turbo::from_chrono(tp)));
            REQUIRE_EQ(tp, std::chrono::time_point_cast<
                    std::chrono::system_clock::time_point::duration>(
                    std::chrono::time_point_cast<Timestamp::duration>(tp)));
        }
        Timestamp::duration::rep v = std::numeric_limits<int64_t>::min();
        v *= Timestamp::duration::period::den;
        auto ts = Timestamp(Timestamp::duration(v));
        ts += std::chrono::duration<int64_t, std::atto>(0);
        REQUIRE_EQ(std::numeric_limits<int64_t>::min(),
                  ts.time_since_epoch().count() / Timestamp::duration::period::den);
        REQUIRE_EQ(0,
                  ts.time_since_epoch().count() % Timestamp::duration::period::den);
        v = std::numeric_limits<int64_t>::max();
        v *= Timestamp::duration::period::den;
        ts = Timestamp(Timestamp::duration(v));
        ts += std::chrono::duration<int64_t, std::atto>(999999999750000000);
        REQUIRE_EQ(std::numeric_limits<int64_t>::max(),
                  ts.time_since_epoch().count() / Timestamp::duration::period::den);
        REQUIRE_EQ(999999999750000000,
                  ts.time_since_epoch().count() % Timestamp::duration::period::den);
    }

    TEST_CASE("Time, TimeZoneAt") {
        const turbo::TimeZone nyc =
                turbo::time_internal::load_time_zone("America/New_York");
        const std::string fmt = "%a, %e %b %Y %H:%M:%S %z (%Z)";

        // A non-transition where the civil time is unique.
        turbo::CivilSecond nov01(2013, 11, 1, 8, 30, 0);
        const auto nov01_ci = nyc.At(nov01);
        REQUIRE_EQ(turbo::TimeZone::TimeInfo::UNIQUE, nov01_ci.kind);
        REQUIRE_EQ("Fri,  1 Nov 2013 08:30:00 -0400 (EDT)",
                  turbo::format_time(fmt, nov01_ci.pre, nyc));
        REQUIRE_EQ(nov01_ci.pre, nov01_ci.trans);
        REQUIRE_EQ(nov01_ci.pre, nov01_ci.post);
        REQUIRE_EQ(nov01_ci.pre, turbo::from_civil(nov01, nyc));

        // A Spring DST transition, when there is a gap in civil time
        // and we prefer the later of the possible interpretations of a
        // non-existent time.
        turbo::CivilSecond mar13(2011, 3, 13, 2, 15, 0);
        const auto mar_ci = nyc.At(mar13);
        REQUIRE_EQ(turbo::TimeZone::TimeInfo::SKIPPED, mar_ci.kind);
        REQUIRE_EQ("Sun, 13 Mar 2011 03:15:00 -0400 (EDT)",
                  turbo::format_time(fmt, mar_ci.pre, nyc));
        REQUIRE_EQ("Sun, 13 Mar 2011 03:00:00 -0400 (EDT)",
                  turbo::format_time(fmt, mar_ci.trans, nyc));
        REQUIRE_EQ("Sun, 13 Mar 2011 01:15:00 -0500 (EST)",
                  turbo::format_time(fmt, mar_ci.post, nyc));
        REQUIRE_EQ(mar_ci.trans, turbo::from_civil(mar13, nyc));

        // A Fall DST transition, when civil times are repeated and
        // we prefer the earlier of the possible interpretations of an
        // ambiguous time.
        turbo::CivilSecond nov06(2011, 11, 6, 1, 15, 0);
        const auto nov06_ci = nyc.At(nov06);
        REQUIRE_EQ(turbo::TimeZone::TimeInfo::REPEATED, nov06_ci.kind);
        REQUIRE_EQ("Sun,  6 Nov 2011 01:15:00 -0400 (EDT)",
                  turbo::format_time(fmt, nov06_ci.pre, nyc));
        REQUIRE_EQ("Sun,  6 Nov 2011 01:00:00 -0500 (EST)",
                  turbo::format_time(fmt, nov06_ci.trans, nyc));
        REQUIRE_EQ("Sun,  6 Nov 2011 01:15:00 -0500 (EST)",
                  turbo::format_time(fmt, nov06_ci.post, nyc));
        REQUIRE_EQ(nov06_ci.pre, turbo::from_civil(nov06, nyc));

        // Check that (time_t) -1 is handled correctly.
        turbo::CivilSecond minus1(1969, 12, 31, 18, 59, 59);
        const auto minus1_cl = nyc.At(minus1);
        REQUIRE_EQ(turbo::TimeZone::TimeInfo::UNIQUE, minus1_cl.kind);
        REQUIRE_EQ(-1, turbo::to_time_t(minus1_cl.pre));
        REQUIRE_EQ("Wed, 31 Dec 1969 18:59:59 -0500 (EST)",
                  turbo::format_time(fmt, minus1_cl.pre, nyc));
        REQUIRE_EQ("Wed, 31 Dec 1969 23:59:59 +0000 (UTC)",
                  turbo::format_time(fmt, minus1_cl.pre, turbo::utc_time_zone()));
    }

// from_civil(CivilSecond(year, mon, day, hour, min, sec), utc_time_zone())
// has a specialized fastpath implementation, which we exercise here.
    TEST_CASE("Time, FromCivilUTC") {
        const turbo::TimeZone utc = turbo::utc_time_zone();
        const std::string fmt = "%a, %e %b %Y %H:%M:%S %z (%Z)";
        const int kMax = std::numeric_limits<int>::max();
        const int kMin = std::numeric_limits<int>::min();
        turbo::Time t;

        // 292091940881 is the last positive year to use the fastpath.
        t = turbo::from_civil(
                turbo::CivilSecond(292091940881, kMax, kMax, kMax, kMax, kMax), utc);
        REQUIRE_EQ("Fri, 25 Nov 292277026596 12:21:07 +0000 (UTC)",
                  turbo::format_time(fmt, t, utc));
        t = turbo::from_civil(
                turbo::CivilSecond(292091940882, kMax, kMax, kMax, kMax, kMax), utc);
        REQUIRE_EQ("infinite-future", turbo::format_time(fmt, t, utc));  // no overflow

        // -292091936940 is the last negative year to use the fastpath.
        t = turbo::from_civil(
                turbo::CivilSecond(-292091936940, kMin, kMin, kMin, kMin, kMin), utc);
        REQUIRE_EQ("Fri,  1 Nov -292277022657 10:37:52 +0000 (UTC)",
                  turbo::format_time(fmt, t, utc));
        t = turbo::from_civil(
                turbo::CivilSecond(-292091936941, kMin, kMin, kMin, kMin, kMin), utc);
        REQUIRE_EQ("infinite-past", turbo::format_time(fmt, t, utc));  // no underflow

        // Check that we're counting leap years correctly.
        t = turbo::from_civil(turbo::CivilSecond(1900, 2, 28, 23, 59, 59), utc);
        REQUIRE_EQ("Wed, 28 Feb 1900 23:59:59 +0000 (UTC)",
                  turbo::format_time(fmt, t, utc));
        t = turbo::from_civil(turbo::CivilSecond(1900, 3, 1, 0, 0, 0), utc);
        REQUIRE_EQ("Thu,  1 Mar 1900 00:00:00 +0000 (UTC)",
                  turbo::format_time(fmt, t, utc));
        t = turbo::from_civil(turbo::CivilSecond(2000, 2, 29, 23, 59, 59), utc);
        REQUIRE_EQ("Tue, 29 Feb 2000 23:59:59 +0000 (UTC)",
                  turbo::format_time(fmt, t, utc));
        t = turbo::from_civil(turbo::CivilSecond(2000, 3, 1, 0, 0, 0), utc);
        REQUIRE_EQ("Wed,  1 Mar 2000 00:00:00 +0000 (UTC)",
                  turbo::format_time(fmt, t, utc));
    }

    TEST_CASE("time, to_tm") {
        const turbo::TimeZone utc = turbo::utc_time_zone();

        // Compares the results of to_tm() to gmtime_r() for lots of times over the
        // course of a few days.
        const turbo::Time start =
                turbo::from_civil(turbo::CivilSecond(2014, 1, 2, 3, 4, 5), utc);
        const turbo::Time end =
                turbo::from_civil(turbo::CivilSecond(2014, 1, 5, 3, 4, 5), utc);
        for (turbo::Time t = start; t < end; t += turbo::seconds(30)) {
            const struct tm tm_bt = to_tm(t, utc);
            const time_t tt = turbo::to_time_t(t);
            struct tm tm_lc;
#ifdef _WIN32
            gmtime_s(&tm_lc, &tt);
#else
            gmtime_r(&tt, &tm_lc);
#endif
            REQUIRE_EQ(tm_lc.tm_year, tm_bt.tm_year);
            REQUIRE_EQ(tm_lc.tm_mon, tm_bt.tm_mon);
            REQUIRE_EQ(tm_lc.tm_mday, tm_bt.tm_mday);
            REQUIRE_EQ(tm_lc.tm_hour, tm_bt.tm_hour);
            REQUIRE_EQ(tm_lc.tm_min, tm_bt.tm_min);
            REQUIRE_EQ(tm_lc.tm_sec, tm_bt.tm_sec);
            REQUIRE_EQ(tm_lc.tm_wday, tm_bt.tm_wday);
            REQUIRE_EQ(tm_lc.tm_yday, tm_bt.tm_yday);
            REQUIRE_EQ(tm_lc.tm_isdst, tm_bt.tm_isdst);
            
        }

        // Checks that the tm_isdst field is correct when in standard time.
        const turbo::TimeZone nyc =
                turbo::time_internal::load_time_zone("America/New_York");
        turbo::Time t = turbo::from_civil(turbo::CivilSecond(2014, 3, 1, 0, 0, 0), nyc);
        struct tm tm = to_tm(t, nyc);
        REQUIRE_FALSE(tm.tm_isdst);

        // Checks that the tm_isdst field is correct when in daylight time.
        t = turbo::from_civil(turbo::CivilSecond(2014, 4, 1, 0, 0, 0), nyc);
        tm = to_tm(t, nyc);
        REQUIRE(tm.tm_isdst);

        // Checks overflow.
        tm = to_tm(turbo::infinite_future(), nyc);
        REQUIRE_EQ(std::numeric_limits<int>::max() - 1900, tm.tm_year);
        REQUIRE_EQ(11, tm.tm_mon);
        REQUIRE_EQ(31, tm.tm_mday);
        REQUIRE_EQ(23, tm.tm_hour);
        REQUIRE_EQ(59, tm.tm_min);
        REQUIRE_EQ(59, tm.tm_sec);
        REQUIRE_EQ(4, tm.tm_wday);
        REQUIRE_EQ(364, tm.tm_yday);
        REQUIRE_FALSE(tm.tm_isdst);

        // Checks underflow.
        tm = to_tm(turbo::infinite_past(), nyc);
        REQUIRE_EQ(std::numeric_limits<int>::min(), tm.tm_year);
        REQUIRE_EQ(0, tm.tm_mon);
        REQUIRE_EQ(1, tm.tm_mday);
        REQUIRE_EQ(0, tm.tm_hour);
        REQUIRE_EQ(0, tm.tm_min);
        REQUIRE_EQ(0, tm.tm_sec);
        REQUIRE_EQ(0, tm.tm_wday);
        REQUIRE_EQ(0, tm.tm_yday);
        REQUIRE_FALSE(tm.tm_isdst);
    }

    TEST_CASE("time, from_tm") {
        const turbo::TimeZone nyc =
                turbo::time_internal::load_time_zone("America/New_York");

        // Verifies that tm_isdst doesn't affect anything when the time is unique.
        struct tm tm;
        std::memset(&tm, 0, sizeof(tm));
        tm.tm_year = 2014 - 1900;
        tm.tm_mon = 6 - 1;
        tm.tm_mday = 28;
        tm.tm_hour = 1;
        tm.tm_min = 2;
        tm.tm_sec = 3;
        tm.tm_isdst = -1;
        turbo::Time t = from_tm(tm, nyc);
        REQUIRE_EQ("2014-06-28T01:02:03-04:00", turbo::format_time(t, nyc));  // DST
        tm.tm_isdst = 0;
        t = from_tm(tm, nyc);
        REQUIRE_EQ("2014-06-28T01:02:03-04:00", turbo::format_time(t, nyc));  // DST
        tm.tm_isdst = 1;
        t = from_tm(tm, nyc);
        REQUIRE_EQ("2014-06-28T01:02:03-04:00", turbo::format_time(t, nyc));  // DST

        // Adjusts tm to refer to an ambiguous time.
        tm.tm_year = 2014 - 1900;
        tm.tm_mon = 11 - 1;
        tm.tm_mday = 2;
        tm.tm_hour = 1;
        tm.tm_min = 30;
        tm.tm_sec = 42;
        tm.tm_isdst = -1;
        t = from_tm(tm, nyc);
        REQUIRE_EQ("2014-11-02T01:30:42-04:00", turbo::format_time(t, nyc));  // DST
        tm.tm_isdst = 0;
        t = from_tm(tm, nyc);
        REQUIRE_EQ("2014-11-02T01:30:42-05:00", turbo::format_time(t, nyc));  // STD
        tm.tm_isdst = 1;
        t = from_tm(tm, nyc);
        REQUIRE_EQ("2014-11-02T01:30:42-04:00", turbo::format_time(t, nyc));  // DST

        // Adjusts tm to refer to a skipped time.
        tm.tm_year = 2014 - 1900;
        tm.tm_mon = 3 - 1;
        tm.tm_mday = 9;
        tm.tm_hour = 2;
        tm.tm_min = 30;
        tm.tm_sec = 42;
        tm.tm_isdst = -1;
        t = from_tm(tm, nyc);
        REQUIRE_EQ("2014-03-09T03:30:42-04:00", turbo::format_time(t, nyc));  // DST
        tm.tm_isdst = 0;
        t = from_tm(tm, nyc);
        REQUIRE_EQ("2014-03-09T01:30:42-05:00", turbo::format_time(t, nyc));  // STD
        tm.tm_isdst = 1;
        t = from_tm(tm, nyc);
        REQUIRE_EQ("2014-03-09T03:30:42-04:00", turbo::format_time(t, nyc));  // DST

        // Adjusts tm to refer to a time with a year larger than 2147483647.
        tm.tm_year = 2147483647 - 1900 + 1;
        tm.tm_mon = 6 - 1;
        tm.tm_mday = 28;
        tm.tm_hour = 1;
        tm.tm_min = 2;
        tm.tm_sec = 3;
        tm.tm_isdst = -1;
        t = from_tm(tm, turbo::utc_time_zone());
        REQUIRE_EQ("2147483648-06-28T01:02:03+00:00",
                  turbo::format_time(t, turbo::utc_time_zone()));

        // Adjusts tm to refer to a time with a very large month.
        tm.tm_year = 2019 - 1900;
        tm.tm_mon = 2147483647;
        tm.tm_mday = 28;
        tm.tm_hour = 1;
        tm.tm_min = 2;
        tm.tm_sec = 3;
        tm.tm_isdst = -1;
        t = from_tm(tm, turbo::utc_time_zone());
        REQUIRE_EQ("178958989-08-28T01:02:03+00:00",
                  turbo::format_time(t, turbo::utc_time_zone()));
    }

    TEST_CASE("Time, TMRoundTrip") {
        const turbo::TimeZone nyc =
                turbo::time_internal::load_time_zone("America/New_York");

        // Test round-tripping across a skipped transition
        turbo::Time start = turbo::from_civil(turbo::CivilHour(2014, 3, 9, 0), nyc);
        turbo::Time end = turbo::from_civil(turbo::CivilHour(2014, 3, 9, 4), nyc);
        for (turbo::Time t = start; t < end; t += turbo::minutes(1)) {
            struct tm tm = to_tm(t, nyc);
            turbo::Time rt = from_tm(tm, nyc);
            REQUIRE_EQ(rt, t);
        }

        // Test round-tripping across an ambiguous transition
        start = turbo::from_civil(turbo::CivilHour(2014, 11, 2, 0), nyc);
        end = turbo::from_civil(turbo::CivilHour(2014, 11, 2, 4), nyc);
        for (turbo::Time t = start; t < end; t += turbo::minutes(1)) {
            struct tm tm = to_tm(t, nyc);
            turbo::Time rt = from_tm(tm, nyc);
            REQUIRE_EQ(rt, t);
        }

        // Test round-tripping of unique instants crossing a day boundary
        start = turbo::from_civil(turbo::CivilHour(2014, 6, 27, 22), nyc);
        end = turbo::from_civil(turbo::CivilHour(2014, 6, 28, 4), nyc);
        for (turbo::Time t = start; t < end; t += turbo::minutes(1)) {
            struct tm tm = to_tm(t, nyc);
            turbo::Time rt = from_tm(tm, nyc);
            REQUIRE_EQ(rt, t);
        }
    }

    TEST_CASE("Time, Range") {
        // The API's documented range is +/- 100 billion years.
        const turbo::Duration range = turbo::hours(24) * 365.2425 * 100000000000;

        // Arithmetic and comparison still works at +/-range around base values.
        turbo::Time bases[2] = {turbo::unix_epoch(), turbo::time_now()};
        for (const auto base: bases) {
            turbo::Time bottom = base - range;
            REQUIRE_GT(bottom, bottom - turbo::nanoseconds(1));
            REQUIRE_LT(bottom, bottom + turbo::nanoseconds(1));
            turbo::Time top = base + range;
            REQUIRE_GT(top, top - turbo::nanoseconds(1));
            REQUIRE_LT(top, top + turbo::nanoseconds(1));
            turbo::Duration full_range = 2 * range;
            REQUIRE_EQ(full_range, top - bottom);
            REQUIRE_EQ(-full_range, bottom - top);
        }
    }

    TEST_CASE("Time, Limits") {
        // It is an implementation detail that Time().rep_ == zero_duration(),
        // and that the resolution of a Duration is 1/4 of a nanosecond.
        const turbo::Time zero;
        const turbo::Time max =
                zero + turbo::seconds(std::numeric_limits<int64_t>::max()) +
                turbo::nanoseconds(999999999) + turbo::nanoseconds(3) / 4;
        const turbo::Time min =
                zero + turbo::seconds(std::numeric_limits<int64_t>::min());

        // Some simple max/min bounds checks.
        REQUIRE_LT(max, turbo::infinite_future());
        REQUIRE_GT(min, turbo::infinite_past());
        REQUIRE_LT(zero, max);
        REQUIRE_GT(zero, min);
        REQUIRE_GE(turbo::unix_epoch(), min);
        REQUIRE_LT(turbo::unix_epoch(), max);

        // Check sign of Time differences.
        REQUIRE_LT(turbo::zero_duration(), max - zero);
        REQUIRE_LT(turbo::zero_duration(),
                  zero - turbo::nanoseconds(1) / 4 - min);  // avoid zero - min

        // Arithmetic works at max - 0.25ns and min + 0.25ns.
        REQUIRE_GT(max, max - turbo::nanoseconds(1) / 4);
        REQUIRE_LT(min, min + turbo::nanoseconds(1) / 4);
    }

    TEST_CASE("Time, ConversionSaturation") {
        const turbo::TimeZone utc = turbo::utc_time_zone();
        turbo::Time t;

        const auto max_time_t = std::numeric_limits<time_t>::max();
        const auto min_time_t = std::numeric_limits<time_t>::min();
        time_t tt = max_time_t - 1;
        t = turbo::from_time_t(tt);
        tt = turbo::to_time_t(t);
        REQUIRE_EQ(max_time_t - 1, tt);
        t += turbo::seconds(1);
        tt = turbo::to_time_t(t);
        REQUIRE_EQ(max_time_t, tt);
        t += turbo::seconds(1);  // no effect
        tt = turbo::to_time_t(t);
        REQUIRE_EQ(max_time_t, tt);

        tt = min_time_t + 1;
        t = turbo::from_time_t(tt);
        tt = turbo::to_time_t(t);
        REQUIRE_EQ(min_time_t + 1, tt);
        t -= turbo::seconds(1);
        tt = turbo::to_time_t(t);
        REQUIRE_EQ(min_time_t, tt);
        t -= turbo::seconds(1);  // no effect
        tt = turbo::to_time_t(t);
        REQUIRE_EQ(min_time_t, tt);

        const auto max_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::max();
        const auto min_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::min();
        timeval tv;
        tv.tv_sec = max_timeval_sec;
        tv.tv_usec = 999998;
        t = turbo::time_from_timeval(tv);
        tv = to_timeval(t);
        REQUIRE_EQ(max_timeval_sec, tv.tv_sec);
        REQUIRE_EQ(999998, tv.tv_usec);
        t += turbo::microseconds(1);
        tv = to_timeval(t);
        REQUIRE_EQ(max_timeval_sec, tv.tv_sec);
        REQUIRE_EQ(999999, tv.tv_usec);
        t += turbo::microseconds(1);  // no effect
        tv = to_timeval(t);
        REQUIRE_EQ(max_timeval_sec, tv.tv_sec);
        REQUIRE_EQ(999999, tv.tv_usec);

        tv.tv_sec = min_timeval_sec;
        tv.tv_usec = 1;
        t = turbo::time_from_timeval(tv);
        tv = to_timeval(t);
        REQUIRE_EQ(min_timeval_sec, tv.tv_sec);
        REQUIRE_EQ(1, tv.tv_usec);
        t -= turbo::microseconds(1);
        tv = to_timeval(t);
        REQUIRE_EQ(min_timeval_sec, tv.tv_sec);
        REQUIRE_EQ(0, tv.tv_usec);
        t -= turbo::microseconds(1);  // no effect
        tv = to_timeval(t);
        REQUIRE_EQ(min_timeval_sec, tv.tv_sec);
        REQUIRE_EQ(0, tv.tv_usec);

        const auto max_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::max();
        const auto min_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::min();
        timespec ts;
        ts.tv_sec = max_timespec_sec;
        ts.tv_nsec = 999999998;
        t = turbo::time_from_timespec(ts);
        ts = turbo::to_timespec(t);
        REQUIRE_EQ(max_timespec_sec, ts.tv_sec);
        REQUIRE_EQ(999999998, ts.tv_nsec);
        t += turbo::nanoseconds(1);
        ts = turbo::to_timespec(t);
        REQUIRE_EQ(max_timespec_sec, ts.tv_sec);
        REQUIRE_EQ(999999999, ts.tv_nsec);
        t += turbo::nanoseconds(1);  // no effect
        ts = turbo::to_timespec(t);
        REQUIRE_EQ(max_timespec_sec, ts.tv_sec);
        REQUIRE_EQ(999999999, ts.tv_nsec);

        ts.tv_sec = min_timespec_sec;
        ts.tv_nsec = 1;
        t = turbo::time_from_timespec(ts);
        ts = turbo::to_timespec(t);
        REQUIRE_EQ(min_timespec_sec, ts.tv_sec);
        REQUIRE_EQ(1, ts.tv_nsec);
        t -= turbo::nanoseconds(1);
        ts = turbo::to_timespec(t);
        REQUIRE_EQ(min_timespec_sec, ts.tv_sec);
        REQUIRE_EQ(0, ts.tv_nsec);
        t -= turbo::nanoseconds(1);  // no effect
        ts = turbo::to_timespec(t);
        REQUIRE_EQ(min_timespec_sec, ts.tv_sec);
        REQUIRE_EQ(0, ts.tv_nsec);

        // Checks how TimeZone::At() saturates on infinities.
        auto ci = utc.At(turbo::infinite_future());
        REQUIRE_CIVIL_INFO(ci, std::numeric_limits<int64_t>::max(), 12, 31, 23, 59, 59,
                          0, false);
        REQUIRE_EQ(turbo::infinite_duration(), ci.subsecond);
        REQUIRE_EQ(turbo::Weekday::thursday, turbo::get_weekday(ci.cs));
        REQUIRE_EQ(365, turbo::get_year_day(ci.cs));
        REQUIRE_EQ("-00", ci.zone_abbr);  // artifact of TimeZone::At()
        ci = utc.At(turbo::infinite_past());
        REQUIRE_CIVIL_INFO(ci, std::numeric_limits<int64_t>::min(), 1, 1, 0, 0, 0, 0,
                          false);
        REQUIRE_EQ(-turbo::infinite_duration(), ci.subsecond);
        REQUIRE_EQ(turbo::Weekday::sunday, turbo::get_weekday(ci.cs));
        REQUIRE_EQ(1, turbo::get_year_day(ci.cs));
        REQUIRE_EQ("-00", ci.zone_abbr);  // artifact of TimeZone::At()

        // Approach the maximal Time value from below.
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 4, 15, 30, 6), utc);
        REQUIRE_EQ("292277026596-12-04T15:30:06+00:00",
                  turbo::format_time(turbo::RFC3339_full, t, utc));
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 4, 15, 30, 7), utc);
        REQUIRE_EQ("292277026596-12-04T15:30:07+00:00",
                  turbo::format_time(turbo::RFC3339_full, t, utc));
        REQUIRE_EQ(
                turbo::unix_epoch() + turbo::seconds(std::numeric_limits<int64_t>::max()),
                t);

        // Checks that we can also get the maximal Time value for a far-east zone.
        const turbo::TimeZone plus14 = turbo::fixed_time_zone(14 * 60 * 60);
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 5, 5, 30, 7), plus14);
        REQUIRE_EQ("292277026596-12-05T05:30:07+14:00",
                  turbo::format_time(turbo::RFC3339_full, t, plus14));
        REQUIRE_EQ(
                turbo::unix_epoch() + turbo::seconds(std::numeric_limits<int64_t>::max()),
                t);

        // One second later should push us to infinity.
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 4, 15, 30, 8), utc);
        REQUIRE_EQ("infinite-future", turbo::format_time(turbo::RFC3339_full, t, utc));

        // Approach the minimal Time value from above.
        t = turbo::from_civil(turbo::CivilSecond(-292277022657, 1, 27, 8, 29, 53), utc);
        REQUIRE_EQ("-292277022657-01-27T08:29:53+00:00",
                  turbo::format_time(turbo::RFC3339_full, t, utc));
        t = turbo::from_civil(turbo::CivilSecond(-292277022657, 1, 27, 8, 29, 52), utc);
        REQUIRE_EQ("-292277022657-01-27T08:29:52+00:00",
                  turbo::format_time(turbo::RFC3339_full, t, utc));
        REQUIRE_EQ(
                turbo::unix_epoch() + turbo::seconds(std::numeric_limits<int64_t>::min()),
                t);

        // Checks that we can also get the minimal Time value for a far-west zone.
        const turbo::TimeZone minus12 = turbo::fixed_time_zone(-12 * 60 * 60);
        t = turbo::from_civil(turbo::CivilSecond(-292277022657, 1, 26, 20, 29, 52),
                             minus12);
        REQUIRE_EQ("-292277022657-01-26T20:29:52-12:00",
                  turbo::format_time(turbo::RFC3339_full, t, minus12));
        REQUIRE_EQ(
                turbo::unix_epoch() + turbo::seconds(std::numeric_limits<int64_t>::min()),
                t);

        // One second before should push us to -infinity.
        t = turbo::from_civil(turbo::CivilSecond(-292277022657, 1, 27, 8, 29, 51), utc);
        REQUIRE_EQ("infinite-past", turbo::format_time(turbo::RFC3339_full, t, utc));
    }

// In zones with POSIX-style recurring rules we use special logic to
// handle conversions in the distant future.  Here we check the limits
// of those conversions, particularly with respect to integer overflow.
    TEST_CASE("Time, ExtendedConversionSaturation") {
        const turbo::TimeZone syd =
                turbo::time_internal::load_time_zone("Australia/Sydney");
        const turbo::TimeZone nyc =
                turbo::time_internal::load_time_zone("America/New_York");
        const turbo::Time max =
                turbo::from_unix_seconds(std::numeric_limits<int64_t>::max());
        turbo::TimeZone::CivilInfo ci;
        turbo::Time t;

        // The maximal time converted in each zone.
        ci = syd.At(max);
        REQUIRE_CIVIL_INFO(ci, 292277026596, 12, 5, 2, 30, 7, 39600, true);
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 5, 2, 30, 7), syd);
        REQUIRE_EQ(max, t);
        ci = nyc.At(max);
        REQUIRE_CIVIL_INFO(ci, 292277026596, 12, 4, 10, 30, 7, -18000, false);
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 4, 10, 30, 7), nyc);
        REQUIRE_EQ(max, t);

        // One second later should push us to infinity.
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 5, 2, 30, 8), syd);
        REQUIRE_EQ(turbo::infinite_future(), t);
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 4, 10, 30, 8), nyc);
        REQUIRE_EQ(turbo::infinite_future(), t);

        // And we should stick there.
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 5, 2, 30, 9), syd);
        REQUIRE_EQ(turbo::infinite_future(), t);
        t = turbo::from_civil(turbo::CivilSecond(292277026596, 12, 4, 10, 30, 9), nyc);
        REQUIRE_EQ(turbo::infinite_future(), t);

        // All the way up to a saturated date/time, without overflow.
        t = turbo::from_civil(turbo::CivilSecond::max(), syd);
        REQUIRE_EQ(turbo::infinite_future(), t);
        t = turbo::from_civil(turbo::CivilSecond::max(), nyc);
        REQUIRE_EQ(turbo::infinite_future(), t);
    }

    TEST_CASE("Time, FromCivilAlignment") {
        const turbo::TimeZone utc = turbo::utc_time_zone();
        const turbo::CivilSecond cs(2015, 2, 3, 4, 5, 6);
        turbo::Time t = turbo::from_civil(cs, utc);
        REQUIRE_EQ("2015-02-03T04:05:06+00:00", turbo::format_time(t, utc));
        t = turbo::from_civil(turbo::CivilMinute(cs), utc);
        REQUIRE_EQ("2015-02-03T04:05:00+00:00", turbo::format_time(t, utc));
        t = turbo::from_civil(turbo::CivilHour(cs), utc);
        REQUIRE_EQ("2015-02-03T04:00:00+00:00", turbo::format_time(t, utc));
        t = turbo::from_civil(turbo::CivilDay(cs), utc);
        REQUIRE_EQ("2015-02-03T00:00:00+00:00", turbo::format_time(t, utc));
        t = turbo::from_civil(turbo::CivilMonth(cs), utc);
        REQUIRE_EQ("2015-02-01T00:00:00+00:00", turbo::format_time(t, utc));
        t = turbo::from_civil(turbo::CivilYear(cs), utc);
        REQUIRE_EQ("2015-01-01T00:00:00+00:00", turbo::format_time(t, utc));
    }

    TEST_CASE("Time, LegacyDateTime") {
        const turbo::TimeZone utc = turbo::utc_time_zone();
        const std::string ymdhms = "%Y-%m-%d %H:%M:%S";
        const int kMax = std::numeric_limits<int>::max();
        const int kMin = std::numeric_limits<int>::min();
        turbo::Time t;

        t = turbo::from_date_time(std::numeric_limits<turbo::civil_year_t>::max(), kMax,
                                kMax, kMax, kMax, kMax, utc);
        REQUIRE_EQ("infinite-future",
                  turbo::format_time(ymdhms, t, utc));  // no overflow
        t = turbo::from_date_time(std::numeric_limits<turbo::civil_year_t>::min(), kMin,
                                kMin, kMin, kMin, kMin, utc);
        REQUIRE_EQ("infinite-past", turbo::format_time(ymdhms, t, utc));  // no overflow

        // Check normalization.
        REQUIRE(turbo::convert_date_time(2013, 10, 32, 8, 30, 0, utc).normalized);
        t = turbo::from_date_time(2015, 1, 1, 0, 0, 60, utc);
        REQUIRE_EQ("2015-01-01 00:01:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, 1, 1, 0, 60, 0, utc);
        REQUIRE_EQ("2015-01-01 01:00:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, 1, 1, 24, 0, 0, utc);
        REQUIRE_EQ("2015-01-02 00:00:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, 1, 32, 0, 0, 0, utc);
        REQUIRE_EQ("2015-02-01 00:00:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, 13, 1, 0, 0, 0, utc);
        REQUIRE_EQ("2016-01-01 00:00:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, 13, 32, 60, 60, 60, utc);
        REQUIRE_EQ("2016-02-03 13:01:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, 1, 1, 0, 0, -1, utc);
        REQUIRE_EQ("2014-12-31 23:59:59", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, 1, 1, 0, -1, 0, utc);
        REQUIRE_EQ("2014-12-31 23:59:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, 1, 1, -1, 0, 0, utc);
        REQUIRE_EQ("2014-12-31 23:00:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, 1, -1, 0, 0, 0, utc);
        REQUIRE_EQ("2014-12-30 00:00:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, -1, 1, 0, 0, 0, utc);
        REQUIRE_EQ("2014-11-01 00:00:00", turbo::format_time(ymdhms, t, utc));
        t = turbo::from_date_time(2015, -1, -1, -1, -1, -1, utc);
        REQUIRE_EQ("2014-10-29 22:58:59", turbo::format_time(ymdhms, t, utc));
    }

    TEST_CASE("Time, NextTransitionUTC") {
        const auto tz = turbo::utc_time_zone();
        turbo::TimeZone::CivilTransition trans;

        auto t = turbo::infinite_past();
        REQUIRE_FALSE(tz.NextTransition(t, &trans));

        t = turbo::infinite_future();
        REQUIRE_FALSE(tz.NextTransition(t, &trans));
    }

    TEST_CASE("Time, PrevTransitionUTC") {
        const auto tz = turbo::utc_time_zone();
        turbo::TimeZone::CivilTransition trans;

        auto t = turbo::infinite_future();
        REQUIRE_FALSE(tz.PrevTransition(t, &trans));

        t = turbo::infinite_past();
        REQUIRE_FALSE(tz.PrevTransition(t, &trans));
    }

    TEST_CASE("Time, NextTransitionNYC") {
        const auto tz = turbo::time_internal::load_time_zone("America/New_York");
        turbo::TimeZone::CivilTransition trans;

        auto t = turbo::from_civil(turbo::CivilSecond(2018, 6, 30, 0, 0, 0), tz);
        REQUIRE(tz.NextTransition(t, &trans));
        REQUIRE_EQ(turbo::CivilSecond(2018, 11, 4, 2, 0, 0), trans.from);
        REQUIRE_EQ(turbo::CivilSecond(2018, 11, 4, 1, 0, 0), trans.to);

        t = turbo::infinite_future();
        REQUIRE_FALSE(tz.NextTransition(t, &trans));

        t = turbo::infinite_past();
        REQUIRE(tz.NextTransition(t, &trans));
        if (trans.from == turbo::CivilSecond(1918, 03, 31, 2, 0, 0)) {
            // It looks like the tzdata is only 32 bit (probably macOS),
            // which bottoms out at 1901-12-13T20:45:52+00:00.
            REQUIRE_EQ(turbo::CivilSecond(1918, 3, 31, 3, 0, 0), trans.to);
        } else {
            REQUIRE_EQ(turbo::CivilSecond(1883, 11, 18, 12, 3, 58), trans.from);
            REQUIRE_EQ(turbo::CivilSecond(1883, 11, 18, 12, 0, 0), trans.to);
        }
    }

    TEST_CASE("Time, PrevTransitionNYC") {
        const auto tz = turbo::time_internal::load_time_zone("America/New_York");
        turbo::TimeZone::CivilTransition trans;

        auto t = turbo::from_civil(turbo::CivilSecond(2018, 6, 30, 0, 0, 0), tz);
        REQUIRE(tz.PrevTransition(t, &trans));
        REQUIRE_EQ(turbo::CivilSecond(2018, 3, 11, 2, 0, 0), trans.from);
        REQUIRE_EQ(turbo::CivilSecond(2018, 3, 11, 3, 0, 0), trans.to);

        t = turbo::infinite_past();
        REQUIRE_FALSE(tz.PrevTransition(t, &trans));

        t = turbo::infinite_future();
        REQUIRE(tz.PrevTransition(t, &trans));
        // We have a transition but we don't know which one.
    }

}  // namespace
