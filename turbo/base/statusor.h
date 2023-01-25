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
//
// -----------------------------------------------------------------------------
// File: statusor.h
// -----------------------------------------------------------------------------
//
// An `turbo::StatusOr<T>` represents a union of an `turbo::Status` object
// and an object of type `T`. The `turbo::StatusOr<T>` will either contain an
// object of type `T` (indicating a successful operation), or an error (of type
// `turbo::Status`) explaining why such a value is not present.
//
// In general, check the success of an operation returning an
// `turbo::StatusOr<T>` like you would an `turbo::Status` by using the `ok()`
// member function.
//
// Example:
//
//   StatusOr<Foo> result = Calculation();
//   if (result.ok()) {
//     result->DoSomethingCool();
//   } else {
//     LOG(ERROR) << result.status();
//   }
#ifndef TURBO_STATUS_STATUSOR_H_
#define TURBO_STATUS_STATUSOR_H_

#include <exception>
#include <initializer_list>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

#include "turbo/platform/attributes.h"
#include "turbo/platform/call_once.h"
#include "turbo/meta/type_traits.h"
#include "turbo/base/internal/statusor_internal.h"
#include "status.h"
#include "turbo/meta/variant.h"
#include "turbo/meta/utility.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

// BadStatusOrAccess
//
// This class defines the type of object to throw (if exceptions are enabled),
// when accessing the value of an `turbo::StatusOr<T>` object that does not
// contain a value. This behavior is analogous to that of
// `std::bad_optional_access` in the case of accessing an invalid
// `std::optional` value.
//
// Example:
//
// try {
//   turbo::StatusOr<int> v = FetchInt();
//   DoWork(v.value());  // Accessing value() when not "OK" may throw
// } catch (turbo::BadStatusOrAccess& ex) {
//   LOG(ERROR) << ex.status();
// }
class BadStatusOrAccess : public std::exception {
 public:
  explicit BadStatusOrAccess(turbo::Status status);
  ~BadStatusOrAccess() override = default;

  BadStatusOrAccess(const BadStatusOrAccess& other);
  BadStatusOrAccess& operator=(const BadStatusOrAccess& other);
  BadStatusOrAccess(BadStatusOrAccess&& other);
  BadStatusOrAccess& operator=(BadStatusOrAccess&& other);

  // BadStatusOrAccess::what()
  //
  // Returns the associated explanatory string of the `turbo::StatusOr<T>`
  // object's error code. This function contains information about the failing
  // status, but its exact formatting may change and should not be depended on.
  //
  // The pointer of this string is guaranteed to be valid until any non-const
  // function is invoked on the exception object.
  const char* what() const noexcept override;

  // BadStatusOrAccess::status()
  //
  // Returns the associated `turbo::Status` of the `turbo::StatusOr<T>` object's
  // error.
  const turbo::Status& status() const;

 private:
  void InitWhat() const;

  turbo::Status status_;
  mutable turbo::once_flag init_what_;
  mutable std::string what_;
};

// Returned StatusOr objects may not be ignored.
template <typename T>
#if TURBO_HAVE_CPP_ATTRIBUTE(nodiscard)
// TODO(b/176172494): TURBO_MUST_USE_RESULT should expand to the more strict
// [[nodiscard]]. For now, just use [[nodiscard]] directly when it is available.
class [[nodiscard]] StatusOr;
#else
class TURBO_MUST_USE_RESULT StatusOr;
#endif  // TURBO_HAVE_CPP_ATTRIBUTE(nodiscard)

// turbo::StatusOr<T>
//
// The `turbo::StatusOr<T>` class template is a union of an `turbo::Status` object
// and an object of type `T`. The `turbo::StatusOr<T>` models an object that is
// either a usable object, or an error (of type `turbo::Status`) explaining why
// such an object is not present. An `turbo::StatusOr<T>` is typically the return
// value of a function which may fail.
//
// An `turbo::StatusOr<T>` can never hold an "OK" status (an
// `turbo::StatusCode::kOk` value); instead, the presence of an object of type
// `T` indicates success. Instead of checking for a `kOk` value, use the
// `turbo::StatusOr<T>::ok()` member function. (It is for this reason, and code
// readability, that using the `ok()` function is preferred for `turbo::Status`
// as well.)
//
// Example:
//
//   StatusOr<Foo> result = DoBigCalculationThatCouldFail();
//   if (result.ok()) {
//     result->DoSomethingCool();
//   } else {
//     LOG(ERROR) << result.status();
//   }
//
// Accessing the object held by an `turbo::StatusOr<T>` should be performed via
// `operator*` or `operator->`, after a call to `ok()` confirms that the
// `turbo::StatusOr<T>` holds an object of type `T`:
//
// Example:
//
//   turbo::StatusOr<int> i = GetCount();
//   if (i.ok()) {
//     updated_total += *i
//   }
//
// NOTE: using `turbo::StatusOr<T>::value()` when no valid value is present will
// throw an exception if exceptions are enabled or terminate the process when
// exceptions are not enabled.
//
// Example:
//
//   StatusOr<Foo> result = DoBigCalculationThatCouldFail();
//   const Foo& foo = result.value();    // Crash/exception if no value present
//   foo.DoSomethingCool();
//
// A `turbo::StatusOr<T*>` can be constructed from a null pointer like any other
// pointer value, and the result will be that `ok()` returns `true` and
// `value()` returns `nullptr`. Checking the value of pointer in an
// `turbo::StatusOr<T*>` generally requires a bit more care, to ensure both that
// a value is present and that value is not null:
//
//  StatusOr<std::unique_ptr<Foo>> result = FooFactory::MakeNewFoo(arg);
//  if (!result.ok()) {
//    LOG(ERROR) << result.status();
//  } else if (*result == nullptr) {
//    LOG(ERROR) << "Unexpected null pointer";
//  } else {
//    (*result)->DoSomethingCool();
//  }
//
// Example factory implementation returning StatusOr<T>:
//
//  StatusOr<Foo> FooFactory::MakeFoo(int arg) {
//    if (arg <= 0) {
//      return turbo::Status(turbo::StatusCode::kInvalidArgument,
//                          "Arg must be positive");
//    }
//    return Foo(arg);
//  }
template <typename T>
class StatusOr : private internal_statusor::StatusOrData<T>,
                 private internal_statusor::CopyCtorBase<T>,
                 private internal_statusor::MoveCtorBase<T>,
                 private internal_statusor::CopyAssignBase<T>,
                 private internal_statusor::MoveAssignBase<T> {
  template <typename U>
  friend class StatusOr;

  typedef internal_statusor::StatusOrData<T> Base;

 public:
  // StatusOr<T>::value_type
  //
  // This instance data provides a generic `value_type` member for use within
  // generic programming. This usage is analogous to that of
  // `optional::value_type` in the case of `std::optional`.
  typedef T value_type;

  // Constructors

  // Constructs a new `turbo::StatusOr` with an `turbo::StatusCode::kUnknown`
  // status. This constructor is marked 'explicit' to prevent usages in return
  // values such as 'return {};', under the misconception that
  // `turbo::StatusOr<std::vector<int>>` will be initialized with an empty
  // vector, instead of an `turbo::StatusCode::kUnknown` error code.
  explicit StatusOr();

  // `StatusOr<T>` is copy constructible if `T` is copy constructible.
  StatusOr(const StatusOr&) = default;
  // `StatusOr<T>` is copy assignable if `T` is copy constructible and copy
  // assignable.
  StatusOr& operator=(const StatusOr&) = default;

  // `StatusOr<T>` is move constructible if `T` is move constructible.
  StatusOr(StatusOr&&) = default;
  // `StatusOr<T>` is moveAssignable if `T` is move constructible and move
  // assignable.
  StatusOr& operator=(StatusOr&&) = default;

  // Converting Constructors

  // Constructs a new `turbo::StatusOr<T>` from an `turbo::StatusOr<U>`, when `T`
  // is constructible from `U`. To avoid ambiguity, these constructors are
  // disabled if `T` is also constructible from `StatusOr<U>.`. This constructor
  // is explicit if and only if the corresponding construction of `T` from `U`
  // is explicit. (This constructor inherits its explicitness from the
  // underlying constructor.)
  template <
      typename U,
      turbo::enable_if_t<
          turbo::conjunction<
              turbo::negation<std::is_same<T, U>>,
              std::is_constructible<T, const U&>,
              std::is_convertible<const U&, T>,
              turbo::negation<
                  internal_statusor::IsConstructibleOrConvertibleFromStatusOr<
                      T, U>>>::value,
          int> = 0>
  StatusOr(const StatusOr<U>& other)  // NOLINT
      : Base(static_cast<const typename StatusOr<U>::Base&>(other)) {}
  template <
      typename U,
      turbo::enable_if_t<
          turbo::conjunction<
              turbo::negation<std::is_same<T, U>>,
              std::is_constructible<T, const U&>,
              turbo::negation<std::is_convertible<const U&, T>>,
              turbo::negation<
                  internal_statusor::IsConstructibleOrConvertibleFromStatusOr<
                      T, U>>>::value,
          int> = 0>
  explicit StatusOr(const StatusOr<U>& other)
      : Base(static_cast<const typename StatusOr<U>::Base&>(other)) {}

  template <
      typename U,
      turbo::enable_if_t<
          turbo::conjunction<
              turbo::negation<std::is_same<T, U>>, std::is_constructible<T, U&&>,
              std::is_convertible<U&&, T>,
              turbo::negation<
                  internal_statusor::IsConstructibleOrConvertibleFromStatusOr<
                      T, U>>>::value,
          int> = 0>
  StatusOr(StatusOr<U>&& other)  // NOLINT
      : Base(static_cast<typename StatusOr<U>::Base&&>(other)) {}
  template <
      typename U,
      turbo::enable_if_t<
          turbo::conjunction<
              turbo::negation<std::is_same<T, U>>, std::is_constructible<T, U&&>,
              turbo::negation<std::is_convertible<U&&, T>>,
              turbo::negation<
                  internal_statusor::IsConstructibleOrConvertibleFromStatusOr<
                      T, U>>>::value,
          int> = 0>
  explicit StatusOr(StatusOr<U>&& other)
      : Base(static_cast<typename StatusOr<U>::Base&&>(other)) {}

  // Converting Assignment Operators

  // Creates an `turbo::StatusOr<T>` through assignment from an
  // `turbo::StatusOr<U>` when:
  //
  //   * Both `turbo::StatusOr<T>` and `turbo::StatusOr<U>` are OK by assigning
  //     `U` to `T` directly.
  //   * `turbo::StatusOr<T>` is OK and `turbo::StatusOr<U>` contains an error
  //      code by destroying `turbo::StatusOr<T>`'s value and assigning from
  //      `turbo::StatusOr<U>'
  //   * `turbo::StatusOr<T>` contains an error code and `turbo::StatusOr<U>` is
  //      OK by directly initializing `T` from `U`.
  //   * Both `turbo::StatusOr<T>` and `turbo::StatusOr<U>` contain an error
  //     code by assigning the `Status` in `turbo::StatusOr<U>` to
  //     `turbo::StatusOr<T>`
  //
  // These overloads only apply if `turbo::StatusOr<T>` is constructible and
  // assignable from `turbo::StatusOr<U>` and `StatusOr<T>` cannot be directly
  // assigned from `StatusOr<U>`.
  template <
      typename U,
      turbo::enable_if_t<
          turbo::conjunction<
              turbo::negation<std::is_same<T, U>>,
              std::is_constructible<T, const U&>,
              std::is_assignable<T, const U&>,
              turbo::negation<
                  internal_statusor::
                      IsConstructibleOrConvertibleOrAssignableFromStatusOr<
                          T, U>>>::value,
          int> = 0>
  StatusOr& operator=(const StatusOr<U>& other) {
    this->Assign(other);
    return *this;
  }
  template <
      typename U,
      turbo::enable_if_t<
          turbo::conjunction<
              turbo::negation<std::is_same<T, U>>, std::is_constructible<T, U&&>,
              std::is_assignable<T, U&&>,
              turbo::negation<
                  internal_statusor::
                      IsConstructibleOrConvertibleOrAssignableFromStatusOr<
                          T, U>>>::value,
          int> = 0>
  StatusOr& operator=(StatusOr<U>&& other) {
    this->Assign(std::move(other));
    return *this;
  }

  // Constructs a new `turbo::StatusOr<T>` with a non-ok status. After calling
  // this constructor, `this->ok()` will be `false` and calls to `value()` will
  // crash, or produce an exception if exceptions are enabled.
  //
  // The constructor also takes any type `U` that is convertible to
  // `turbo::Status`. This constructor is explicit if an only if `U` is not of
  // type `turbo::Status` and the conversion from `U` to `Status` is explicit.
  //
  // REQUIRES: !Status(std::forward<U>(v)).ok(). This requirement is DCHECKed.
  // In optimized builds, passing turbo::OkStatus() here will have the effect
  // of passing turbo::StatusCode::kInternal as a fallback.
  template <
      typename U = turbo::Status,
      turbo::enable_if_t<
          turbo::conjunction<
              std::is_convertible<U&&, turbo::Status>,
              std::is_constructible<turbo::Status, U&&>,
              turbo::negation<std::is_same<turbo::decay_t<U>, turbo::StatusOr<T>>>,
              turbo::negation<std::is_same<turbo::decay_t<U>, T>>,
              turbo::negation<std::is_same<turbo::decay_t<U>, turbo::in_place_t>>,
              turbo::negation<internal_statusor::HasConversionOperatorToStatusOr<
                  T, U&&>>>::value,
          int> = 0>
  StatusOr(U&& v) : Base(std::forward<U>(v)) {}

  template <
      typename U = turbo::Status,
      turbo::enable_if_t<
          turbo::conjunction<
              turbo::negation<std::is_convertible<U&&, turbo::Status>>,
              std::is_constructible<turbo::Status, U&&>,
              turbo::negation<std::is_same<turbo::decay_t<U>, turbo::StatusOr<T>>>,
              turbo::negation<std::is_same<turbo::decay_t<U>, T>>,
              turbo::negation<std::is_same<turbo::decay_t<U>, turbo::in_place_t>>,
              turbo::negation<internal_statusor::HasConversionOperatorToStatusOr<
                  T, U&&>>>::value,
          int> = 0>
  explicit StatusOr(U&& v) : Base(std::forward<U>(v)) {}

  template <
      typename U = turbo::Status,
      turbo::enable_if_t<
          turbo::conjunction<
              std::is_convertible<U&&, turbo::Status>,
              std::is_constructible<turbo::Status, U&&>,
              turbo::negation<std::is_same<turbo::decay_t<U>, turbo::StatusOr<T>>>,
              turbo::negation<std::is_same<turbo::decay_t<U>, T>>,
              turbo::negation<std::is_same<turbo::decay_t<U>, turbo::in_place_t>>,
              turbo::negation<internal_statusor::HasConversionOperatorToStatusOr<
                  T, U&&>>>::value,
          int> = 0>
  StatusOr& operator=(U&& v) {
    this->AssignStatus(std::forward<U>(v));
    return *this;
  }

  // Perfect-forwarding value assignment operator.

  // If `*this` contains a `T` value before the call, the contained value is
  // assigned from `std::forward<U>(v)`; Otherwise, it is directly-initialized
  // from `std::forward<U>(v)`.
  // This function does not participate in overload unless:
  // 1. `std::is_constructible_v<T, U>` is true,
  // 2. `std::is_assignable_v<T&, U>` is true.
  // 3. `std::is_same_v<StatusOr<T>, std::remove_cvref_t<U>>` is false.
  // 4. Assigning `U` to `T` is not ambiguous:
  //  If `U` is `StatusOr<V>` and `T` is constructible and assignable from
  //  both `StatusOr<V>` and `V`, the assignment is considered bug-prone and
  //  ambiguous thus will fail to compile. For example:
  //    StatusOr<bool> s1 = true;  // s1.ok() && *s1 == true
  //    StatusOr<bool> s2 = false;  // s2.ok() && *s2 == false
  //    s1 = s2;  // ambiguous, `s1 = *s2` or `s1 = bool(s2)`?
  template <
      typename U = T,
      typename = typename std::enable_if<turbo::conjunction<
          std::is_constructible<T, U&&>, std::is_assignable<T&, U&&>,
          turbo::disjunction<
              std::is_same<turbo::remove_cv_t<turbo::remove_reference_t<U>>, T>,
              turbo::conjunction<
                  turbo::negation<std::is_convertible<U&&, turbo::Status>>,
                  turbo::negation<internal_statusor::
                                     HasConversionOperatorToStatusOr<T, U&&>>>>,
          internal_statusor::IsForwardingAssignmentValid<T, U&&>>::value>::type>
  StatusOr& operator=(U&& v) {
    this->Assign(std::forward<U>(v));
    return *this;
  }

  // Constructs the inner value `T` in-place using the provided args, using the
  // `T(args...)` constructor.
  template <typename... Args>
  explicit StatusOr(turbo::in_place_t, Args&&... args);
  template <typename U, typename... Args>
  explicit StatusOr(turbo::in_place_t, std::initializer_list<U> ilist,
                    Args&&... args);

  // Constructs the inner value `T` in-place using the provided args, using the
  // `T(U)` (direct-initialization) constructor. This constructor is only valid
  // if `T` can be constructed from a `U`. Can accept move or copy constructors.
  //
  // This constructor is explicit if `U` is not convertible to `T`. To avoid
  // ambiguity, this constructor is disabled if `U` is a `StatusOr<J>`, where
  // `J` is convertible to `T`.
  template <
      typename U = T,
      turbo::enable_if_t<
          turbo::conjunction<
              internal_statusor::IsDirectInitializationValid<T, U&&>,
              std::is_constructible<T, U&&>, std::is_convertible<U&&, T>,
              turbo::disjunction<
                  std::is_same<turbo::remove_cv_t<turbo::remove_reference_t<U>>,
                               T>,
                  turbo::conjunction<
                      turbo::negation<std::is_convertible<U&&, turbo::Status>>,
                      turbo::negation<
                          internal_statusor::HasConversionOperatorToStatusOr<
                              T, U&&>>>>>::value,
          int> = 0>
  StatusOr(U&& u)  // NOLINT
      : StatusOr(turbo::in_place, std::forward<U>(u)) {}

  template <
      typename U = T,
      turbo::enable_if_t<
          turbo::conjunction<
              internal_statusor::IsDirectInitializationValid<T, U&&>,
              turbo::disjunction<
                  std::is_same<turbo::remove_cv_t<turbo::remove_reference_t<U>>,
                               T>,
                  turbo::conjunction<
                      turbo::negation<std::is_constructible<turbo::Status, U&&>>,
                      turbo::negation<
                          internal_statusor::HasConversionOperatorToStatusOr<
                              T, U&&>>>>,
              std::is_constructible<T, U&&>,
              turbo::negation<std::is_convertible<U&&, T>>>::value,
          int> = 0>
  explicit StatusOr(U&& u)  // NOLINT
      : StatusOr(turbo::in_place, std::forward<U>(u)) {}

  // StatusOr<T>::ok()
  //
  // Returns whether or not this `turbo::StatusOr<T>` holds a `T` value. This
  // member function is analogous to `turbo::Status::ok()` and should be used
  // similarly to check the status of return values.
  //
  // Example:
  //
  // StatusOr<Foo> result = DoBigCalculationThatCouldFail();
  // if (result.ok()) {
  //    // Handle result
  // else {
  //    // Handle error
  // }
  TURBO_MUST_USE_RESULT bool ok() const { return this->status_.ok(); }

  // StatusOr<T>::status()
  //
  // Returns a reference to the current `turbo::Status` contained within the
  // `turbo::StatusOr<T>`. If `turbo::StatusOr<T>` contains a `T`, then this
  // function returns `turbo::OkStatus()`.
  const Status& status() const&;
  Status status() &&;

  // StatusOr<T>::value()
  //
  // Returns a reference to the held value if `this->ok()`. Otherwise, throws
  // `turbo::BadStatusOrAccess` if exceptions are enabled, or is guaranteed to
  // terminate the process if exceptions are disabled.
  //
  // If you have already checked the status using `this->ok()`, you probably
  // want to use `operator*()` or `operator->()` to access the value instead of
  // `value`.
  //
  // Note: for value types that are cheap to copy, prefer simple code:
  //
  //   T value = statusor.value();
  //
  // Otherwise, if the value type is expensive to copy, but can be left
  // in the StatusOr, simply assign to a reference:
  //
  //   T& value = statusor.value();  // or `const T&`
  //
  // Otherwise, if the value type supports an efficient move, it can be
  // used as follows:
  //
  //   T value = std::move(statusor).value();
  //
  // The `std::move` on statusor instead of on the whole expression enables
  // warnings about possible uses of the statusor object after the move.
  const T& value() const& TURBO_ATTRIBUTE_LIFETIME_BOUND;
  T& value() & TURBO_ATTRIBUTE_LIFETIME_BOUND;
  const T&& value() const&& TURBO_ATTRIBUTE_LIFETIME_BOUND;
  T&& value() && TURBO_ATTRIBUTE_LIFETIME_BOUND;

  // StatusOr<T>:: operator*()
  //
  // Returns a reference to the current value.
  //
  // REQUIRES: `this->ok() == true`, otherwise the behavior is undefined.
  //
  // Use `this->ok()` to verify that there is a current value within the
  // `turbo::StatusOr<T>`. Alternatively, see the `value()` member function for a
  // similar API that guarantees crashing or throwing an exception if there is
  // no current value.
  const T& operator*() const& TURBO_ATTRIBUTE_LIFETIME_BOUND;
  T& operator*() & TURBO_ATTRIBUTE_LIFETIME_BOUND;
  const T&& operator*() const&& TURBO_ATTRIBUTE_LIFETIME_BOUND;
  T&& operator*() && TURBO_ATTRIBUTE_LIFETIME_BOUND;

  // StatusOr<T>::operator->()
  //
  // Returns a pointer to the current value.
  //
  // REQUIRES: `this->ok() == true`, otherwise the behavior is undefined.
  //
  // Use `this->ok()` to verify that there is a current value.
  const T* operator->() const TURBO_ATTRIBUTE_LIFETIME_BOUND;
  T* operator->() TURBO_ATTRIBUTE_LIFETIME_BOUND;

  // StatusOr<T>::value_or()
  //
  // Returns the current value if `this->ok() == true`. Otherwise constructs a
  // value using the provided `default_value`.
  //
  // Unlike `value`, this function returns by value, copying the current value
  // if necessary. If the value type supports an efficient move, it can be used
  // as follows:
  //
  //   T value = std::move(statusor).value_or(def);
  //
  // Unlike with `value`, calling `std::move()` on the result of `value_or` will
  // still trigger a copy.
  template <typename U>
  T value_or(U&& default_value) const&;
  template <typename U>
  T value_or(U&& default_value) &&;

  // StatusOr<T>::IgnoreError()
  //
  // Ignores any errors. This method does nothing except potentially suppress
  // complaints from any tools that are checking that errors are not dropped on
  // the floor.
  void IgnoreError() const;

  // StatusOr<T>::emplace()
  //
  // Reconstructs the inner value T in-place using the provided args, using the
  // T(args...) constructor. Returns reference to the reconstructed `T`.
  template <typename... Args>
  T& emplace(Args&&... args) {
    if (ok()) {
      this->Clear();
      this->MakeValue(std::forward<Args>(args)...);
    } else {
      this->MakeValue(std::forward<Args>(args)...);
      this->status_ = turbo::OkStatus();
    }
    return this->data_;
  }

  template <
      typename U, typename... Args,
      turbo::enable_if_t<
          std::is_constructible<T, std::initializer_list<U>&, Args&&...>::value,
          int> = 0>
  T& emplace(std::initializer_list<U> ilist, Args&&... args) {
    if (ok()) {
      this->Clear();
      this->MakeValue(ilist, std::forward<Args>(args)...);
    } else {
      this->MakeValue(ilist, std::forward<Args>(args)...);
      this->status_ = turbo::OkStatus();
    }
    return this->data_;
  }

 private:
  using internal_statusor::StatusOrData<T>::Assign;
  template <typename U>
  void Assign(const turbo::StatusOr<U>& other);
  template <typename U>
  void Assign(turbo::StatusOr<U>&& other);
};

// operator==()
//
// This operator checks the equality of two `turbo::StatusOr<T>` objects.
template <typename T>
bool operator==(const StatusOr<T>& lhs, const StatusOr<T>& rhs) {
  if (lhs.ok() && rhs.ok()) return *lhs == *rhs;
  return lhs.status() == rhs.status();
}

// operator!=()
//
// This operator checks the inequality of two `turbo::StatusOr<T>` objects.
template <typename T>
bool operator!=(const StatusOr<T>& lhs, const StatusOr<T>& rhs) {
  return !(lhs == rhs);
}

//------------------------------------------------------------------------------
// Implementation details for StatusOr<T>
//------------------------------------------------------------------------------

// TODO(sbenza): avoid the string here completely.
template <typename T>
StatusOr<T>::StatusOr() : Base(Status(turbo::StatusCode::kUnknown, "")) {}

template <typename T>
template <typename U>
inline void StatusOr<T>::Assign(const StatusOr<U>& other) {
  if (other.ok()) {
    this->Assign(*other);
  } else {
    this->AssignStatus(other.status());
  }
}

template <typename T>
template <typename U>
inline void StatusOr<T>::Assign(StatusOr<U>&& other) {
  if (other.ok()) {
    this->Assign(*std::move(other));
  } else {
    this->AssignStatus(std::move(other).status());
  }
}
template <typename T>
template <typename... Args>
StatusOr<T>::StatusOr(turbo::in_place_t, Args&&... args)
    : Base(turbo::in_place, std::forward<Args>(args)...) {}

template <typename T>
template <typename U, typename... Args>
StatusOr<T>::StatusOr(turbo::in_place_t, std::initializer_list<U> ilist,
                      Args&&... args)
    : Base(turbo::in_place, ilist, std::forward<Args>(args)...) {}

template <typename T>
const Status& StatusOr<T>::status() const& {
  return this->status_;
}
template <typename T>
Status StatusOr<T>::status() && {
  return ok() ? OkStatus() : std::move(this->status_);
}

template <typename T>
const T& StatusOr<T>::value() const& {
  if (!this->ok()) internal_statusor::ThrowBadStatusOrAccess(this->status_);
  return this->data_;
}

template <typename T>
T& StatusOr<T>::value() & {
  if (!this->ok()) internal_statusor::ThrowBadStatusOrAccess(this->status_);
  return this->data_;
}

template <typename T>
const T&& StatusOr<T>::value() const&& {
  if (!this->ok()) {
    internal_statusor::ThrowBadStatusOrAccess(std::move(this->status_));
  }
  return std::move(this->data_);
}

template <typename T>
T&& StatusOr<T>::value() && {
  if (!this->ok()) {
    internal_statusor::ThrowBadStatusOrAccess(std::move(this->status_));
  }
  return std::move(this->data_);
}

template <typename T>
const T& StatusOr<T>::operator*() const& {
  this->EnsureOk();
  return this->data_;
}

template <typename T>
T& StatusOr<T>::operator*() & {
  this->EnsureOk();
  return this->data_;
}

template <typename T>
const T&& StatusOr<T>::operator*() const&& {
  this->EnsureOk();
  return std::move(this->data_);
}

template <typename T>
T&& StatusOr<T>::operator*() && {
  this->EnsureOk();
  return std::move(this->data_);
}

template <typename T>
const T* StatusOr<T>::operator->() const {
  this->EnsureOk();
  return &this->data_;
}

template <typename T>
T* StatusOr<T>::operator->() {
  this->EnsureOk();
  return &this->data_;
}

template <typename T>
template <typename U>
T StatusOr<T>::value_or(U&& default_value) const& {
  if (ok()) {
    return this->data_;
  }
  return std::forward<U>(default_value);
}

template <typename T>
template <typename U>
T StatusOr<T>::value_or(U&& default_value) && {
  if (ok()) {
    return std::move(this->data_);
  }
  return std::forward<U>(default_value);
}

template <typename T>
void StatusOr<T>::IgnoreError() const {
  // no-op
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STATUS_STATUSOR_H_