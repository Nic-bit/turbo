// Copyright 2019 The Turbo Authors.
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
// File: inlined_vector.h
// -----------------------------------------------------------------------------
//
// This header file contains the declaration and definition of an "inlined
// vector" which behaves in an equivalent fashion to a `std::vector`, except
// that storage for small sequences of the vector are provided inline without
// requiring any heap allocation.
//
// An `turbo::InlinedVector<T, N>` specifies the default capacity `N` as one of
// its template parameters. Instances where `size() <= N` hold contained
// elements in inline space. Typically `N` is very small so that sequences that
// are expected to be short do not require allocations.
//
// An `turbo::InlinedVector` does not usually require a specific allocator. If
// the inlined vector grows beyond its initial constraints, it will need to
// allocate (as any normal `std::vector` would). This is usually performed with
// the default allocator (defined as `std::allocator<T>`). Optionally, a custom
// allocator type may be specified as `A` in `turbo::InlinedVector<T, N, A>`.

#ifndef TURBO_CONTAINER_INLINED_VECTOR_H_
#define TURBO_CONTAINER_INLINED_VECTOR_H_

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "turbo/base/internal/throw_delegate.h"
#include "turbo/container/internal/inlined_vector.h"
#include "turbo/memory/memory.h"
#include "turbo/meta/algorithm.h"
#include "turbo/meta/type_traits.h"
#include "turbo/platform/port.h"

namespace turbo {
// -----------------------------------------------------------------------------
// InlinedVector
// -----------------------------------------------------------------------------
//
// An `turbo::InlinedVector` is designed to be a drop-in replacement for
// `std::vector` for use cases where the vector's size is sufficiently small
// that it can be inlined. If the inlined vector does grow beyond its estimated
// capacity, it will trigger an initial allocation on the heap, and will behave
// as a `std::vector`. The API of the `turbo::InlinedVector` within this file is
// designed to cover the same API footprint as covered by `std::vector`.
    template<typename T, size_t N, typename A = std::allocator<T>>
    class InlinedVector {
        static_assert(N > 0, "`turbo::InlinedVector` requires an inlined capacity.");

        using Storage = inlined_vector_internal::Storage<T, N, A>;

        template<typename TheA>
        using AllocatorTraits = inlined_vector_internal::AllocatorTraits<TheA>;
        template<typename TheA>
        using MoveIterator = inlined_vector_internal::MoveIterator<TheA>;
        template<typename TheA>
        using IsMemcpyOk = inlined_vector_internal::IsMemcpyOk<TheA>;
        template<typename TheA>
        using IsMoveAssignOk = inlined_vector_internal::IsMoveAssignOk<TheA>;

        template<typename TheA, typename Iterator>
        using IteratorValueAdapter =
                inlined_vector_internal::IteratorValueAdapter<TheA, Iterator>;
        template<typename TheA>
        using CopyValueAdapter = inlined_vector_internal::CopyValueAdapter<TheA>;
        template<typename TheA>
        using DefaultValueAdapter =
                inlined_vector_internal::DefaultValueAdapter<TheA>;

        template<typename Iterator>
        using EnableIfAtLeastForwardIterator = std::enable_if_t<
                inlined_vector_internal::IsAtLeastForwardIterator<Iterator>::value, int>;
        template<typename Iterator>
        using DisableIfAtLeastForwardIterator = std::enable_if_t<
                !inlined_vector_internal::IsAtLeastForwardIterator<Iterator>::value, int>;

        using MemcpyPolicy = typename Storage::MemcpyPolicy;
        using ElementwiseAssignPolicy = typename Storage::ElementwiseAssignPolicy;
        using ElementwiseConstructPolicy =
                typename Storage::ElementwiseConstructPolicy;
        using MoveAssignmentPolicy = typename Storage::MoveAssignmentPolicy;

    public:
        using allocator_type = A;
        using value_type = inlined_vector_internal::ValueType<A>;
        using pointer = inlined_vector_internal::Pointer<A>;
        using const_pointer = inlined_vector_internal::ConstPointer<A>;
        using size_type = inlined_vector_internal::SizeType<A>;
        using difference_type = inlined_vector_internal::DifferenceType<A>;
        using reference = inlined_vector_internal::Reference<A>;
        using const_reference = inlined_vector_internal::ConstReference<A>;
        using iterator = inlined_vector_internal::Iterator<A>;
        using const_iterator = inlined_vector_internal::ConstIterator<A>;
        using reverse_iterator = inlined_vector_internal::ReverseIterator<A>;
        using const_reverse_iterator =
                inlined_vector_internal::ConstReverseIterator<A>;

        // ---------------------------------------------------------------------------
        // InlinedVector Constructors and Destructor
        // ---------------------------------------------------------------------------

        // Creates an empty inlined vector with a value-initialized allocator.
        InlinedVector() noexcept(noexcept(allocator_type())): storage_() {}

        // Creates an empty inlined vector with a copy of `allocator`.
        explicit InlinedVector(const allocator_type &allocator) noexcept
                : storage_(allocator) {}

        // Creates an inlined vector with `n` copies of `value_type()`.
        explicit InlinedVector(size_type n,
                               const allocator_type &allocator = allocator_type())
                : storage_(allocator) {
            storage_.Initialize(DefaultValueAdapter<A>(), n);
        }

        // Creates an inlined vector with `n` copies of `v`.
        InlinedVector(size_type n, const_reference v,
                      const allocator_type &allocator = allocator_type())
                : storage_(allocator) {
            storage_.Initialize(CopyValueAdapter<A>(std::addressof(v)), n);
        }

        // Creates an inlined vector with copies of the elements of `list`.
        InlinedVector(std::initializer_list<value_type> list,
                      const allocator_type &allocator = allocator_type())
                : InlinedVector(list.begin(), list.end(), allocator) {}

        // Creates an inlined vector with elements constructed from the provided
        // forward iterator range [`first`, `last`).
        //
        // NOTE: the `enable_if` prevents ambiguous interpretation between a call to
        // this constructor with two integral arguments and a call to the above
        // `InlinedVector(size_type, const_reference)` constructor.
        template<typename ForwardIterator,
                EnableIfAtLeastForwardIterator<ForwardIterator> = 0>
        InlinedVector(ForwardIterator first, ForwardIterator last,
                      const allocator_type &allocator = allocator_type())
                : storage_(allocator) {
            storage_.Initialize(IteratorValueAdapter<A, ForwardIterator>(first),
                                static_cast<size_t>(std::distance(first, last)));
        }

        // Creates an inlined vector with elements constructed from the provided input
        // iterator range [`first`, `last`).
        template<typename InputIterator,
                DisableIfAtLeastForwardIterator<InputIterator> = 0>
        InlinedVector(InputIterator first, InputIterator last,
                      const allocator_type &allocator = allocator_type())
                : storage_(allocator) {
            std::copy(first, last, std::back_inserter(*this));
        }

        // Creates an inlined vector by copying the contents of `other` using
        // `other`'s allocator.
        InlinedVector(const InlinedVector &other)
                : InlinedVector(other, other.storage_.GetAllocator()) {}

        // Creates an inlined vector by copying the contents of `other` using the
        // provided `allocator`.
        InlinedVector(const InlinedVector &other, const allocator_type &allocator)
                : storage_(allocator) {
            if (other.empty()) {
                // Empty; nothing to do.
            } else if (IsMemcpyOk<A>::value && !other.storage_.GetIsAllocated()) {
                // Memcpy-able and do not need allocation.
                storage_.MemcpyFrom(other.storage_);
            } else {
                storage_.InitFrom(other.storage_);
            }
        }

        // Creates an inlined vector by moving in the contents of `other` without
        // allocating. If `other` contains allocated memory, the newly-created inlined
        // vector will take ownership of that memory. However, if `other` does not
        // contain allocated memory, the newly-created inlined vector will perform
        // element-wise move construction of the contents of `other`.
        //
        // NOTE: since no allocation is performed for the inlined vector in either
        // case, the `noexcept(...)` specification depends on whether moving the
        // underlying objects can throw. It is assumed assumed that...
        //  a) move constructors should only throw due to allocation failure.
        //  b) if `value_type`'s move constructor allocates, it uses the same
        //     allocation function as the inlined vector's allocator.
        // Thus, the move constructor is non-throwing if the allocator is non-throwing
        // or `value_type`'s move constructor is specified as `noexcept`.
        InlinedVector(InlinedVector &&other) noexcept(
        turbo::allocator_is_nothrow<allocator_type>::value ||
        std::is_nothrow_move_constructible<value_type>::value)
                : storage_(other.storage_.GetAllocator()) {
            if (IsMemcpyOk<A>::value) {
                storage_.MemcpyFrom(other.storage_);

                other.storage_.SetInlinedSize(0);
            } else if (other.storage_.GetIsAllocated()) {
                storage_.SetAllocation({other.storage_.GetAllocatedData(),
                                        other.storage_.GetAllocatedCapacity()});
                storage_.SetAllocatedSize(other.storage_.GetSize());

                other.storage_.SetInlinedSize(0);
            } else {
                IteratorValueAdapter<A, MoveIterator<A>> other_values(
                        MoveIterator<A>(other.storage_.GetInlinedData()));

                inlined_vector_internal::ConstructElements<A>(
                        storage_.GetAllocator(), storage_.GetInlinedData(), other_values,
                        other.storage_.GetSize());

                storage_.SetInlinedSize(other.storage_.GetSize());
            }
        }

        // Creates an inlined vector by moving in the contents of `other` with a copy
        // of `allocator`.
        //
        // NOTE: if `other`'s allocator is not equal to `allocator`, even if `other`
        // contains allocated memory, this move constructor will still allocate. Since
        // allocation is performed, this constructor can only be `noexcept` if the
        // specified allocator is also `noexcept`.
        InlinedVector(
                InlinedVector &&other,
                const allocator_type &
                allocator) noexcept(turbo::allocator_is_nothrow<allocator_type>::value)
                : storage_(allocator) {
            if (IsMemcpyOk<A>::value) {
                storage_.MemcpyFrom(other.storage_);

                other.storage_.SetInlinedSize(0);
            } else if ((storage_.GetAllocator() == other.storage_.GetAllocator()) &&
                       other.storage_.GetIsAllocated()) {
                storage_.SetAllocation({other.storage_.GetAllocatedData(),
                                        other.storage_.GetAllocatedCapacity()});
                storage_.SetAllocatedSize(other.storage_.GetSize());

                other.storage_.SetInlinedSize(0);
            } else {
                storage_.Initialize(IteratorValueAdapter<A, MoveIterator<A>>(
                                            MoveIterator<A>(other.data())),
                                    other.size());
            }
        }

        ~InlinedVector() {}

        // ---------------------------------------------------------------------------
        // InlinedVector Member Accessors
        // ---------------------------------------------------------------------------

        // `InlinedVector::empty()`
        //
        // Returns whether the inlined vector contains no elements.
        bool empty() const noexcept { return !size(); }

        // `InlinedVector::size()`
        //
        // Returns the number of elements in the inlined vector.
        size_type size() const noexcept { return storage_.GetSize(); }

        // `InlinedVector::max_size()`
        //
        // Returns the maximum number of elements the inlined vector can hold.
        size_type max_size() const noexcept {
            // One bit of the size storage is used to indicate whether the inlined
            // vector contains allocated memory. As a result, the maximum size that the
            // inlined vector can express is the minimum of the limit of how many
            // objects we can allocate and std::numeric_limits<size_type>::max() / 2.
            return (std::min)(AllocatorTraits<A>::max_size(storage_.GetAllocator()),
                              (std::numeric_limits<size_type>::max)() / 2);
        }

        // `InlinedVector::capacity()`
        //
        // Returns the number of elements that could be stored in the inlined vector
        // without requiring a reallocation.
        //
        // NOTE: for most inlined vectors, `capacity()` should be equal to the
        // template parameter `N`. For inlined vectors which exceed this capacity,
        // they will no longer be inlined and `capacity()` will equal the capactity of
        // the allocated memory.
        size_type capacity() const noexcept {
            return storage_.GetIsAllocated() ? storage_.GetAllocatedCapacity()
                                             : storage_.GetInlinedCapacity();
        }

        // `InlinedVector::data()`
        //
        // Returns a `pointer` to the elements of the inlined vector. This pointer
        // can be used to access and modify the contained elements.
        //
        // NOTE: only elements within [`data()`, `data() + size()`) are valid.
        pointer data() noexcept {
            return storage_.GetIsAllocated() ? storage_.GetAllocatedData()
                                             : storage_.GetInlinedData();
        }

        // Overload of `InlinedVector::data()` that returns a `const_pointer` to the
        // elements of the inlined vector. This pointer can be used to access but not
        // modify the contained elements.
        //
        // NOTE: only elements within [`data()`, `data() + size()`) are valid.
        const_pointer data() const noexcept {
            return storage_.GetIsAllocated() ? storage_.GetAllocatedData()
                                             : storage_.GetInlinedData();
        }

        // `InlinedVector::operator[](...)`
        //
        // Returns a `reference` to the `i`th element of the inlined vector.
        reference operator[](size_type i) {
            TURBO_HARDENING_ASSERT(i < size());
            return data()[i];
        }

        // Overload of `InlinedVector::operator[](...)` that returns a
        // `const_reference` to the `i`th element of the inlined vector.
        const_reference operator[](size_type i) const {
            TURBO_HARDENING_ASSERT(i < size());
            return data()[i];
        }

        // `InlinedVector::at(...)`
        //
        // Returns a `reference` to the `i`th element of the inlined vector.
        //
        // NOTE: if `i` is not within the required range of `InlinedVector::at(...)`,
        // in both debug and non-debug builds, `std::out_of_range` will be thrown.
        reference at(size_type i) {
            if (TURBO_UNLIKELY(i >= size())) {
                base_internal::ThrowStdOutOfRange(
                        "`InlinedVector::at(size_type)` failed bounds check");
            }
            return data()[i];
        }

        // Overload of `InlinedVector::at(...)` that returns a `const_reference` to
        // the `i`th element of the inlined vector.
        //
        // NOTE: if `i` is not within the required range of `InlinedVector::at(...)`,
        // in both debug and non-debug builds, `std::out_of_range` will be thrown.
        const_reference at(size_type i) const {
            if (TURBO_UNLIKELY(i >= size())) {
                base_internal::ThrowStdOutOfRange(
                        "`InlinedVector::at(size_type) const` failed bounds check");
            }
            return data()[i];
        }

        // `InlinedVector::front()`
        //
        // Returns a `reference` to the first element of the inlined vector.
        reference front() {
            TURBO_HARDENING_ASSERT(!empty());
            return data()[0];
        }

        // Overload of `InlinedVector::front()` that returns a `const_reference` to
        // the first element of the inlined vector.
        const_reference front() const {
            TURBO_HARDENING_ASSERT(!empty());
            return data()[0];
        }

        // `InlinedVector::back()`
        //
        // Returns a `reference` to the last element of the inlined vector.
        reference back() {
            TURBO_HARDENING_ASSERT(!empty());
            return data()[size() - 1];
        }

        // Overload of `InlinedVector::back()` that returns a `const_reference` to the
        // last element of the inlined vector.
        const_reference back() const {
            TURBO_HARDENING_ASSERT(!empty());
            return data()[size() - 1];
        }

        // `InlinedVector::begin()`
        //
        // Returns an `iterator` to the beginning of the inlined vector.
        iterator begin() noexcept { return data(); }

        // Overload of `InlinedVector::begin()` that returns a `const_iterator` to
        // the beginning of the inlined vector.
        const_iterator begin() const noexcept { return data(); }

        // `InlinedVector::end()`
        //
        // Returns an `iterator` to the end of the inlined vector.
        iterator end() noexcept { return data() + size(); }

        // Overload of `InlinedVector::end()` that returns a `const_iterator` to the
        // end of the inlined vector.
        const_iterator end() const noexcept { return data() + size(); }

        // `InlinedVector::cbegin()`
        //
        // Returns a `const_iterator` to the beginning of the inlined vector.
        const_iterator cbegin() const noexcept { return begin(); }

        // `InlinedVector::cend()`
        //
        // Returns a `const_iterator` to the end of the inlined vector.
        const_iterator cend() const noexcept { return end(); }

        // `InlinedVector::rbegin()`
        //
        // Returns a `reverse_iterator` from the end of the inlined vector.
        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

        // Overload of `InlinedVector::rbegin()` that returns a
        // `const_reverse_iterator` from the end of the inlined vector.
        const_reverse_iterator rbegin() const noexcept {
            return const_reverse_iterator(end());
        }

        // `InlinedVector::rend()`
        //
        // Returns a `reverse_iterator` from the beginning of the inlined vector.
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

        // Overload of `InlinedVector::rend()` that returns a `const_reverse_iterator`
        // from the beginning of the inlined vector.
        const_reverse_iterator rend() const noexcept {
            return const_reverse_iterator(begin());
        }

        // `InlinedVector::crbegin()`
        //
        // Returns a `const_reverse_iterator` from the end of the inlined vector.
        const_reverse_iterator crbegin() const noexcept { return rbegin(); }

        // `InlinedVector::crend()`
        //
        // Returns a `const_reverse_iterator` from the beginning of the inlined
        // vector.
        const_reverse_iterator crend() const noexcept { return rend(); }

        // `InlinedVector::get_allocator()`
        //
        // Returns a copy of the inlined vector's allocator.
        allocator_type get_allocator() const { return storage_.GetAllocator(); }

        // ---------------------------------------------------------------------------
        // InlinedVector Member Mutators
        // ---------------------------------------------------------------------------

        // `InlinedVector::operator=(...)`
        //
        // Replaces the elements of the inlined vector with copies of the elements of
        // `list`.
        InlinedVector &operator=(std::initializer_list<value_type> list) {
            assign(list.begin(), list.end());

            return *this;
        }

        // Overload of `InlinedVector::operator=(...)` that replaces the elements of
        // the inlined vector with copies of the elements of `other`.
        InlinedVector &operator=(const InlinedVector &other) {
            if (TURBO_LIKELY(this != std::addressof(other))) {
                const_pointer other_data = other.data();
                assign(other_data, other_data + other.size());
            }

            return *this;
        }

        // Overload of `InlinedVector::operator=(...)` that moves the elements of
        // `other` into the inlined vector.
        //
        // NOTE: as a result of calling this overload, `other` is left in a valid but
        // unspecified state.
        InlinedVector &operator=(InlinedVector &&other) {
            if (TURBO_LIKELY(this != std::addressof(other))) {
                MoveAssignment(MoveAssignmentPolicy{}, std::move(other));
            }

            return *this;
        }

        // `InlinedVector::assign(...)`
        //
        // Replaces the contents of the inlined vector with `n` copies of `v`.
        void assign(size_type n, const_reference v) {
            storage_.Assign(CopyValueAdapter<A>(std::addressof(v)), n);
        }

        // Overload of `InlinedVector::assign(...)` that replaces the contents of the
        // inlined vector with copies of the elements of `list`.
        void assign(std::initializer_list<value_type> list) {
            assign(list.begin(), list.end());
        }

        // Overload of `InlinedVector::assign(...)` to replace the contents of the
        // inlined vector with the range [`first`, `last`).
        //
        // NOTE: this overload is for iterators that are "forward" category or better.
        template<typename ForwardIterator,
                EnableIfAtLeastForwardIterator<ForwardIterator> = 0>
        void assign(ForwardIterator first, ForwardIterator last) {
            storage_.Assign(IteratorValueAdapter<A, ForwardIterator>(first),
                            static_cast<size_t>(std::distance(first, last)));
        }

        // Overload of `InlinedVector::assign(...)` to replace the contents of the
        // inlined vector with the range [`first`, `last`).
        //
        // NOTE: this overload is for iterators that are "input" category.
        template<typename InputIterator,
                DisableIfAtLeastForwardIterator<InputIterator> = 0>
        void assign(InputIterator first, InputIterator last) {
            size_type i = 0;
            for (; i < size() && first != last; ++i, static_cast<void>(++first)) {
                data()[i] = *first;
            }

            erase(data() + i, data() + size());
            std::copy(first, last, std::back_inserter(*this));
        }

        // `InlinedVector::resize(...)`
        //
        // Resizes the inlined vector to contain `n` elements.
        //
        // NOTE: If `n` is smaller than `size()`, extra elements are destroyed. If `n`
        // is larger than `size()`, new elements are value-initialized.
        void resize(size_type n) {
            TURBO_HARDENING_ASSERT(n <= max_size());
            storage_.Resize(DefaultValueAdapter<A>(), n);
        }

        // Overload of `InlinedVector::resize(...)` that resizes the inlined vector to
        // contain `n` elements.
        //
        // NOTE: if `n` is smaller than `size()`, extra elements are destroyed. If `n`
        // is larger than `size()`, new elements are copied-constructed from `v`.
        void resize(size_type n, const_reference v) {
            TURBO_HARDENING_ASSERT(n <= max_size());
            storage_.Resize(CopyValueAdapter<A>(std::addressof(v)), n);
        }

        // `InlinedVector::insert(...)`
        //
        // Inserts a copy of `v` at `pos`, returning an `iterator` to the newly
        // inserted element.
        iterator insert(const_iterator pos, const_reference v) {
            return emplace(pos, v);
        }

        // Overload of `InlinedVector::insert(...)` that inserts `v` at `pos` using
        // move semantics, returning an `iterator` to the newly inserted element.
        iterator insert(const_iterator pos, value_type &&v) {
            return emplace(pos, std::move(v));
        }

        // Overload of `InlinedVector::insert(...)` that inserts `n` contiguous copies
        // of `v` starting at `pos`, returning an `iterator` pointing to the first of
        // the newly inserted elements.
        iterator insert(const_iterator pos, size_type n, const_reference v) {
            TURBO_HARDENING_ASSERT(pos >= begin());
            TURBO_HARDENING_ASSERT(pos <= end());

            if (TURBO_LIKELY(n != 0)) {
                value_type dealias = v;
                // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102329#c2
                // It appears that GCC thinks that since `pos` is a const pointer and may
                // point to uninitialized memory at this point, a warning should be
                // issued. But `pos` is actually only used to compute an array index to
                // write to.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
                return storage_.Insert(pos, CopyValueAdapter<A>(std::addressof(dealias)),
                                       n);
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
            } else {
                return const_cast<iterator>(pos);
            }
        }

        // Overload of `InlinedVector::insert(...)` that inserts copies of the
        // elements of `list` starting at `pos`, returning an `iterator` pointing to
        // the first of the newly inserted elements.
        iterator insert(const_iterator pos, std::initializer_list<value_type> list) {
            return insert(pos, list.begin(), list.end());
        }

        // Overload of `InlinedVector::insert(...)` that inserts the range [`first`,
        // `last`) starting at `pos`, returning an `iterator` pointing to the first
        // of the newly inserted elements.
        //
        // NOTE: this overload is for iterators that are "forward" category or better.
        template<typename ForwardIterator,
                EnableIfAtLeastForwardIterator<ForwardIterator> = 0>
        iterator insert(const_iterator pos, ForwardIterator first,
                        ForwardIterator last) {
            TURBO_HARDENING_ASSERT(pos >= begin());
            TURBO_HARDENING_ASSERT(pos <= end());

            if (TURBO_LIKELY(first != last)) {
                return storage_.Insert(
                        pos, IteratorValueAdapter<A, ForwardIterator>(first),
                        static_cast<size_type>(std::distance(first, last)));
            } else {
                return const_cast<iterator>(pos);
            }
        }

        // Overload of `InlinedVector::insert(...)` that inserts the range [`first`,
        // `last`) starting at `pos`, returning an `iterator` pointing to the first
        // of the newly inserted elements.
        //
        // NOTE: this overload is for iterators that are "input" category.
        template<typename InputIterator,
                DisableIfAtLeastForwardIterator<InputIterator> = 0>
        iterator insert(const_iterator pos, InputIterator first, InputIterator last) {
            TURBO_HARDENING_ASSERT(pos >= begin());
            TURBO_HARDENING_ASSERT(pos <= end());

            size_type index = static_cast<size_type>(std::distance(cbegin(), pos));
            for (size_type i = index; first != last; ++i, static_cast<void>(++first)) {
                insert(data() + i, *first);
            }

            return iterator(data() + index);
        }

        // `InlinedVector::emplace(...)`
        //
        // Constructs and inserts an element using `args...` in the inlined vector at
        // `pos`, returning an `iterator` pointing to the newly emplaced element.
        template<typename... Args>
        iterator emplace(const_iterator pos, Args &&... args) {
            TURBO_HARDENING_ASSERT(pos >= begin());
            TURBO_HARDENING_ASSERT(pos <= end());

            value_type dealias(std::forward<Args>(args)...);
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102329#c2
            // It appears that GCC thinks that since `pos` is a const pointer and may
            // point to uninitialized memory at this point, a warning should be
            // issued. But `pos` is actually only used to compute an array index to
            // write to.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
            return storage_.Insert(pos,
                                   IteratorValueAdapter<A, MoveIterator<A>>(
                                           MoveIterator<A>(std::addressof(dealias))),
                                   1);
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
        }

        // `InlinedVector::emplace_back(...)`
        //
        // Constructs and inserts an element using `args...` in the inlined vector at
        // `end()`, returning a `reference` to the newly emplaced element.
        template<typename... Args>
        reference emplace_back(Args &&... args) {
            return storage_.EmplaceBack(std::forward<Args>(args)...);
        }

        // `InlinedVector::push_back(...)`
        //
        // Inserts a copy of `v` in the inlined vector at `end()`.
        void push_back(const_reference v) { static_cast<void>(emplace_back(v)); }

        // Overload of `InlinedVector::push_back(...)` for inserting `v` at `end()`
        // using move semantics.
        void push_back(value_type &&v) {
            static_cast<void>(emplace_back(std::move(v)));
        }

        // `InlinedVector::pop_back()`
        //
        // Destroys the element at `back()`, reducing the size by `1`.
        void pop_back() noexcept {
            TURBO_HARDENING_ASSERT(!empty());

            AllocatorTraits<A>::destroy(storage_.GetAllocator(), data() + (size() - 1));
            storage_.SubtractSize(1);
        }

        // `InlinedVector::erase(...)`
        //
        // Erases the element at `pos`, returning an `iterator` pointing to where the
        // erased element was located.
        //
        // NOTE: may return `end()`, which is not dereferencable.
        iterator erase(const_iterator pos) {
            TURBO_HARDENING_ASSERT(pos >= begin());
            TURBO_HARDENING_ASSERT(pos < end());

            return storage_.Erase(pos, pos + 1);
        }

        // Overload of `InlinedVector::erase(...)` that erases every element in the
        // range [`from`, `to`), returning an `iterator` pointing to where the first
        // erased element was located.
        //
        // NOTE: may return `end()`, which is not dereferencable.
        iterator erase(const_iterator from, const_iterator to) {
            TURBO_HARDENING_ASSERT(from >= begin());
            TURBO_HARDENING_ASSERT(from <= to);
            TURBO_HARDENING_ASSERT(to <= end());

            if (TURBO_LIKELY(from != to)) {
                return storage_.Erase(from, to);
            } else {
                return const_cast<iterator>(from);
            }
        }

        // `InlinedVector::clear()`
        //
        // Destroys all elements in the inlined vector, setting the size to `0` and
        // deallocating any held memory.
        void clear() noexcept {
            inlined_vector_internal::DestroyAdapter<A>::DestroyElements(
                    storage_.GetAllocator(), data(), size());
            storage_.DeallocateIfAllocated();

            storage_.SetInlinedSize(0);
        }

        // `InlinedVector::reserve(...)`
        //
        // Ensures that there is enough room for at least `n` elements.
        void reserve(size_type n) { storage_.Reserve(n); }

        // `InlinedVector::shrink_to_fit()`
        //
        // Attempts to reduce memory usage by moving elements to (or keeping elements
        // in) the smallest available buffer sufficient for containing `size()`
        // elements.
        //
        // If `size()` is sufficiently small, the elements will be moved into (or kept
        // in) the inlined space.
        void shrink_to_fit() {
            if (storage_.GetIsAllocated()) {
                storage_.ShrinkToFit();
            }
        }

        // `InlinedVector::swap(...)`
        //
        // Swaps the contents of the inlined vector with `other`.
        void swap(InlinedVector &other) {
            if (TURBO_LIKELY(this != std::addressof(other))) {
                storage_.Swap(std::addressof(other.storage_));
            }
        }

    private:
        template<typename H, typename TheT, size_t TheN, typename TheA>
        friend H TurboHashValue(H h, const turbo::InlinedVector<TheT, TheN, TheA> &a);

        void MoveAssignment(MemcpyPolicy, InlinedVector &&other) {
            inlined_vector_internal::DestroyAdapter<A>::DestroyElements(
                    storage_.GetAllocator(), data(), size());
            storage_.DeallocateIfAllocated();
            storage_.MemcpyFrom(other.storage_);

            other.storage_.SetInlinedSize(0);
        }

        void MoveAssignment(ElementwiseAssignPolicy, InlinedVector &&other) {
            if (other.storage_.GetIsAllocated()) {
                MoveAssignment(MemcpyPolicy{}, std::move(other));
            } else {
                storage_.Assign(IteratorValueAdapter<A, MoveIterator<A>>(
                                        MoveIterator<A>(other.storage_.GetInlinedData())),
                                other.size());
            }
        }

        void MoveAssignment(ElementwiseConstructPolicy, InlinedVector &&other) {
            if (other.storage_.GetIsAllocated()) {
                MoveAssignment(MemcpyPolicy{}, std::move(other));
            } else {
                inlined_vector_internal::DestroyAdapter<A>::DestroyElements(
                        storage_.GetAllocator(), data(), size());
                storage_.DeallocateIfAllocated();

                IteratorValueAdapter<A, MoveIterator<A>> other_values(
                        MoveIterator<A>(other.storage_.GetInlinedData()));
                inlined_vector_internal::ConstructElements<A>(
                        storage_.GetAllocator(), storage_.GetInlinedData(), other_values,
                        other.storage_.GetSize());
                storage_.SetInlinedSize(other.storage_.GetSize());
            }
        }

        Storage storage_;
    };

    // -----------------------------------------------------------------------------
    // InlinedVector Non-Member Functions
    // -----------------------------------------------------------------------------

    // `swap(...)`
    //
    // Swaps the contents of two inlined vectors.
    template<typename T, size_t N, typename A>
    void swap(turbo::InlinedVector<T, N, A> &a,
              turbo::InlinedVector<T, N, A> &b) noexcept(noexcept(a.swap(b))) {
        a.swap(b);
    }

    // `operator==(...)`
    //
    // Tests for value-equality of two inlined vectors.
    template<typename T, size_t N, typename A>
    bool operator==(const turbo::InlinedVector<T, N, A> &a,
                    const turbo::InlinedVector<T, N, A> &b) {
        auto a_data = a.data();
        auto b_data = b.data();
        return turbo::equal(a_data, a_data + a.size(), b_data, b_data + b.size());
    }

    // `operator!=(...)`
    //
    // Tests for value-inequality of two inlined vectors.
    template<typename T, size_t N, typename A>
    bool operator!=(const turbo::InlinedVector<T, N, A> &a,
                    const turbo::InlinedVector<T, N, A> &b) {
        return !(a == b);
    }

    // `operator<(...)`
    //
    // Tests whether the value of an inlined vector is less than the value of
    // another inlined vector using a lexicographical comparison algorithm.
    template<typename T, size_t N, typename A>
    bool operator<(const turbo::InlinedVector<T, N, A> &a,
                   const turbo::InlinedVector<T, N, A> &b) {
        auto a_data = a.data();
        auto b_data = b.data();
        return std::lexicographical_compare(a_data, a_data + a.size(), b_data,
                                            b_data + b.size());
    }

    // `operator>(...)`
    //
    // Tests whether the value of an inlined vector is greater than the value of
    // another inlined vector using a lexicographical comparison algorithm.
    template<typename T, size_t N, typename A>
    bool operator>(const turbo::InlinedVector<T, N, A> &a,
                   const turbo::InlinedVector<T, N, A> &b) {
        return b < a;
    }

    // `operator<=(...)`
    //
    // Tests whether the value of an inlined vector is less than or equal to the
    // value of another inlined vector using a lexicographical comparison algorithm.
    template<typename T, size_t N, typename A>
    bool operator<=(const turbo::InlinedVector<T, N, A> &a,
                    const turbo::InlinedVector<T, N, A> &b) {
        return !(b < a);
    }

    // `operator>=(...)`
    //
    // Tests whether the value of an inlined vector is greater than or equal to the
    // value of another inlined vector using a lexicographical comparison algorithm.
    template<typename T, size_t N, typename A>
    bool operator>=(const turbo::InlinedVector<T, N, A> &a,
                    const turbo::InlinedVector<T, N, A> &b) {
        return !(a < b);
    }

    // `TurboHashValue(...)`
    //
    // Provides `turbo::Hash` support for `turbo::InlinedVector`. It is uncommon to
    // call this directly.
    template<typename H, typename T, size_t N, typename A>
    H TurboHashValue(H h, const turbo::InlinedVector<T, N, A> &a) {
        auto size = a.size();
        return H::combine(H::combine_contiguous(std::move(h), a.data(), size), size);
    }

}  // namespace turbo

#endif  // TURBO_CONTAINER_INLINED_VECTOR_H_
