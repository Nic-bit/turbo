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

#include "turbo/meta/type_traits.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest/doctest.h"

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "turbo/platform/port.h"
#include "turbo/times/clock.h"
#include "turbo/times/time.h"

namespace {

    struct Dummy {
    };

    struct ReturnType {
    };

    struct ConvertibleToReturnType {
        operator ReturnType() const;  // NOLINT
    };

    // Unique types used as parameter types for testing the detection idiom.
    struct StructA {
    };
    struct StructB {
    };
    struct StructC {
    };

    struct TypeWithBarFunction {
        template<class T,
                std::enable_if_t<std::is_same<T &&, StructA &>::value, int> = 0>
        ReturnType bar(T &&, const StructB &, StructC &&) &&;  // NOLINT
    };

    struct TypeWithBarFunctionAndConvertibleReturnType {
        template<class T,
                std::enable_if_t<std::is_same<T &&, StructA &>::value, int> = 0>
        ConvertibleToReturnType bar(T &&, const StructB &, StructC &&) &&;  // NOLINT
    };
    

    struct MyTrueType {
        static constexpr bool value = true;
    };

    struct MyFalseType {
        static constexpr bool value = false;
    };


    TEST_CASE("NegationTest, BasicBooleanLogic") {
        REQUIRE_FALSE(std::negation<std::true_type>::value);
        REQUIRE_FALSE(std::negation<MyTrueType>::value);
        REQUIRE(std::negation<std::false_type>::value);
        REQUIRE(std::negation<MyFalseType>::value);
    }

    // all member functions are trivial
    class Trivial {
        int n_;
    };


    TEST_CASE("TypeTraitsTest, TestRemoveCVRef") {
        REQUIRE(
                (std::is_same<typename turbo::remove_cvref<int>::type, int>::value));
        REQUIRE(
                (std::is_same<typename turbo::remove_cvref<int &>::type, int>::value));
        REQUIRE(
                (std::is_same<typename turbo::remove_cvref<int &&>::type, int>::value));
        REQUIRE((
                            std::is_same<typename turbo::remove_cvref<const int &>::type, int>::value));
        REQUIRE(
                (std::is_same<typename turbo::remove_cvref<int *>::type, int *>::value));
        // Does not remove const in this case.
        REQUIRE((std::is_same<typename turbo::remove_cvref<const int *>::type,
                const int *>::value));
        REQUIRE((std::is_same<typename turbo::remove_cvref<int[2]>::type,
                int[2]>::value));
        REQUIRE((std::is_same<typename turbo::remove_cvref<int (&)[2]>::type,
                int[2]>::value));
        REQUIRE((std::is_same<typename turbo::remove_cvref<int (&&)[2]>::type,
                int[2]>::value));
        REQUIRE((std::is_same<typename turbo::remove_cvref<const int[2]>::type,
                int[2]>::value));
        REQUIRE((std::is_same<typename turbo::remove_cvref<const int (&)[2]>::type,
                int[2]>::value));
        REQUIRE((std::is_same<typename turbo::remove_cvref<const int (&&)[2]>::type,
                int[2]>::value));
    }


    struct TypeA {
    };
    struct TypeB {
    };
    struct TypeC {
    };
    struct TypeD {
    };

    template<typename T>
    struct Wrap {
    };

    enum class TypeEnum {
        A, B, C, D
    };

    struct GetTypeT {
        template<typename T,
                std::enable_if_t<std::is_same<T, TypeA>::value, int> = 0>
        TypeEnum operator()(Wrap<T>) const {
            return TypeEnum::A;
        }

        template<typename T,
                std::enable_if_t<std::is_same<T, TypeB>::value, int> = 0>
        TypeEnum operator()(Wrap<T>) const {
            return TypeEnum::B;
        }

        template<typename T,
                std::enable_if_t<std::is_same<T, TypeC>::value, int> = 0>
        TypeEnum operator()(Wrap<T>) const {
            return TypeEnum::C;
        }

        // NOTE: TypeD is intentionally not handled
    } constexpr GetType = {};

    TEST_CASE("TypeTraitsTest, TestEnableIf") {
        REQUIRE_EQ(TypeEnum::A, GetType(Wrap<TypeA>()));
        REQUIRE_EQ(TypeEnum::B, GetType(Wrap<TypeB>()));
        REQUIRE_EQ(TypeEnum::C, GetType(Wrap<TypeC>()));
    }

    struct GetTypeExtT {
        template<typename T>
        std::result_of_t<const GetTypeT &(T)> operator()(T &&arg) const {
            return GetType(std::forward<T>(arg));
        }

        TypeEnum operator()(Wrap<TypeD>) const { return TypeEnum::D; }
    } constexpr GetTypeExt = {};

    TEST_CASE("TypeTraitsTest, TestResultOf") {
        REQUIRE_EQ(TypeEnum::A, GetTypeExt(Wrap<TypeA>()));
        REQUIRE_EQ(TypeEnum::B, GetTypeExt(Wrap<TypeB>()));
        REQUIRE_EQ(TypeEnum::C, GetTypeExt(Wrap<TypeC>()));
        REQUIRE_EQ(TypeEnum::D, GetTypeExt(Wrap<TypeD>()));
    }

    namespace adl_namespace {

        struct DeletedSwap {
        };

        void swap(DeletedSwap &, DeletedSwap &) = delete;

        struct SpecialNoexceptSwap {
            SpecialNoexceptSwap(SpecialNoexceptSwap &&) {}

            SpecialNoexceptSwap &operator=(SpecialNoexceptSwap &&) { return *this; }

            ~SpecialNoexceptSwap() = default;
        };

        void swap(SpecialNoexceptSwap &, SpecialNoexceptSwap &) noexcept {}

    }  // namespace adl_namespace



#ifdef TURBO_HAVE_CONSTANT_EVALUATED

    constexpr int64_t NegateIfConstantEvaluated(int64_t i) {
      if (turbo::is_constant_evaluated()) {
        return -i;
      } else {
        return i;
      }
    }

#endif  // TURBO_HAVE_CONSTANT_EVALUATED

    TEST_CASE("TrivallyRelocatable, is_constant_evaluated") {
#ifdef TURBO_HAVE_CONSTANT_EVALUATED
        constexpr int64_t constant = NegateIfConstantEvaluated(42);
        REQUIRE_EQ(constant, -42);

        int64_t now = turbo::ToUnixSeconds(turbo::Now());
        int64_t not_constant = NegateIfConstantEvaluated(now);
        REQUIRE_EQ(not_constant, now);

        static int64_t const_init = NegateIfConstantEvaluated(42);
        REQUIRE_EQ(const_init, -42);
#else
        INFO("turbo::is_constant_evaluated is not defined");
#endif  // TURBO_HAVE_CONSTANT_EVALUATED
    }


}  // namespace
