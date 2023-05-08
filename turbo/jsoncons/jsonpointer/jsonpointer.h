// Copyright 2013-2023 Daniel Parker
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

#ifndef JSONCONS_JSONPOINTER_JSONPOINTER_HPP
#define JSONCONS_JSONPOINTER_JSONPOINTER_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <iterator>
#include <utility> // std::move
#include <system_error> // system_error
#include <type_traits> // std::enable_if, std::true_type
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/jsonpointer/jsonpointer_error.h"
#include "turbo/jsoncons/detail/write_number.h"

namespace turbo { namespace jsonpointer {

    namespace detail {

    enum class pointer_state 
    {
        start,
        escaped,
        delim
    };

    } // namespace detail

    template <class CharT>
    std::basic_string<CharT> escape_string(const std::basic_string<CharT>& s)
    {
        std::basic_string<CharT> result;
        for (auto c : s)
        {
            switch (c)
            {
                case '~':
                    result.push_back('~');
                    result.push_back('0');
                    break;
                case '/':
                    result.push_back('~');
                    result.push_back('1');
                    break;
                default:
                    result.push_back(c);
                    break;
            }
        }
        return result;
    }

    // basic_json_pointer

    template <class CharT>
    class basic_json_pointer
    {
    public:
        // Member types
        using char_type = CharT;
        using string_type = std::basic_string<char_type>;
        using string_view_type = turbo::basic_string_view<char_type>;
        using const_iterator = typename std::vector<string_type>::const_iterator;
        using iterator = const_iterator;
        using const_reverse_iterator = typename std::vector<string_type>::const_reverse_iterator;
        using reverse_iterator = const_reverse_iterator;
    private:
        std::vector<string_type> tokens_;
    public:
        // Constructors
        basic_json_pointer()
        {
        }

        basic_json_pointer(const std::vector<string_type>& tokens)
            : tokens_(tokens)
        {
        }

        basic_json_pointer(std::vector<string_type>&& tokens)
            : tokens_(std::move(tokens))
        {
        }

        explicit basic_json_pointer(const string_view_type& s)
        {
            std::error_code ec;
            auto jp = parse(s, ec);
            if (ec)
            {
                throw jsonpointer_error(ec);
            }
            tokens_ = std::move(jp.tokens_);
        }

        explicit basic_json_pointer(const string_view_type& s, std::error_code& ec)
        {
            auto jp = parse(s, ec);
            if (!ec)
            {
                tokens_ = std::move(jp.tokens_);
            }
        }

        basic_json_pointer(const basic_json_pointer&) = default;

        basic_json_pointer(basic_json_pointer&&) = default;

        static basic_json_pointer parse(const string_view_type& input, std::error_code& ec)
        {
            std::vector<string_type> tokens;
            if (input.empty() || (input[0] == '#' && input.size() == 1))
            {
                return basic_json_pointer<CharT>();
            }

            const char_type* p;
            const char_type* pend;
            string_type unescaped;
            if (input[0] == '#') 
            {
                unescaped = unescape_uri_string(input, ec);
                p = unescaped.data() + 1;
                pend = unescaped.data() + unescaped.size();
            }
            else
            {
                p = input.data();
                pend = input.data() + input.size();
            }

            auto state = jsonpointer::detail::pointer_state::start;
            string_type buffer;

            while (p < pend)
            {
                bool done = false;
                while (p < pend && !done)
                {
                    switch (state)
                    {
                        case jsonpointer::detail::pointer_state::start: 
                            switch (*p)
                            {
                                case '/':
                                    state = jsonpointer::detail::pointer_state::delim;
                                    break;
                                default:
                                    ec = jsonpointer_errc::expected_slash;
                                    return basic_json_pointer();
                            };
                            break;
                        case jsonpointer::detail::pointer_state::delim: 
                            switch (*p)
                            {
                                case '/':
                                    done = true;
                                    break;
                                case '~':
                                    state = jsonpointer::detail::pointer_state::escaped;
                                    break;
                                default:
                                    buffer.push_back(*p);
                                    break;
                            };
                            break;
                        case jsonpointer::detail::pointer_state::escaped: 
                            switch (*p)
                            {
                                case '0':
                                    buffer.push_back('~');
                                    state = jsonpointer::detail::pointer_state::delim;
                                    break;
                                case '1':
                                    buffer.push_back('/');
                                    state = jsonpointer::detail::pointer_state::delim;
                                    break;
                                default:
                                    ec = jsonpointer_errc::expected_0_or_1;
                                    return basic_json_pointer();
                            };
                            break;
                    }
                    ++p;
                }
                tokens.push_back(buffer);
                buffer.clear();
            }
            if (!buffer.empty())
            {
                tokens.push_back(buffer);
            }
            return basic_json_pointer(tokens);
        }

        static string_type escape_uri_string(const string_type& s)
        {
            string_type escaped;
            for (auto ch : s)
            {
                switch (ch)
                {
                    case '%':
                        escaped.append(string_type{'%','2','5'});
                        break;
                    case '^':
                        escaped.append(string_type{'%','5','E'});
                        break;
                    case '|':
                        escaped.append(string_type{'%','7','C'});
                        break;
                    case '\\':
                        escaped.append(string_type{'%','5','C'});
                        break;
                    case '\"':
                        escaped.append(string_type{'%','2','2'});
                        break;
                    case ' ':
                        escaped.append(string_type{'%','2','0'});
                        break;
                    default:
                        escaped.push_back(ch);
                        break;
                }
            }

            return escaped;
        }

        static string_type unescape_uri_string(const string_view_type& s, std::error_code& ec)
        {
            if (s.size() < 3)
            {
                return string_type(s);
            }
            string_type unescaped;
            std::size_t last = s.size() - 2;
            std::size_t pos = 0;
            while (pos < last)
            {
                if (s[pos] == '%')
                {
                    uint8_t ch;
                    auto result = turbo::detail::to_integer_base16(s.data() + (pos+1), 2, ch);
                    if (!result)
                    {
                        ec = jsonpointer_errc::invalid_uri_escaped_data;
                        return string_type(s);
                    }
                    unescaped.push_back(ch);
                    pos += 3;
                }
                else
                {
                    unescaped.push_back(s[pos]);
                    ++pos;
                }
            }
            while (pos < s.size())
            {
                unescaped.push_back(s[pos]);
                ++pos;
            }
            return unescaped;
        }

        // operator=
        basic_json_pointer& operator=(const basic_json_pointer&) = default;

        basic_json_pointer& operator=(basic_json_pointer&&) = default;

        // Modifiers

        void clear()
        {
            tokens_.clear();
        }

        basic_json_pointer& operator/=(const string_type& s) 
        {
            tokens_.push_back(s);
            return *this;
        }

        template <class IntegerType>
        typename std::enable_if<traits_extension::is_integer<IntegerType>::value, basic_json_pointer&>::type
        operator/=(IntegerType val)
        {
            string_type s;
            turbo::detail::from_integer(val, s);
            tokens_.push_back(s);

            return *this;
        }

        basic_json_pointer& operator+=(const basic_json_pointer& p)
        {
            for (const auto& s : p.tokens_)
            {
                tokens_.push_back(s);
            }
            return *this;
        }

        // Accessors
        bool empty() const
        {
          return tokens_.empty();
        }

#if !defined(JSONCONS_NO_DEPRECATED)

        JSONCONS_DEPRECATED_MSG("Instead, use to_string()")
        string_type string() const
        {
            return to_string();
        }
#endif
        string_type to_string() const
        {
            string_type buffer;
            for (const auto& token : tokens_)
            {
                buffer.push_back('/');
                for (auto c : token)
                {
                    switch (c)
                    {
                        case '~':
                            buffer.push_back('~');
                            buffer.push_back('0');
                            break;
                        case '/':
                            buffer.push_back('~');
                            buffer.push_back('1');
                            break;
                        default:
                            buffer.push_back(c);
                            break;
                    }
                }
            }
            return buffer;
        }

        string_type to_uri_fragment() const
        {
            string_type buffer{'#'};
            for (const auto& token : tokens_)
            {
                buffer.push_back('/');
                string_type s = escape_uri_string(token);
                for (auto c : s)
                {
                    switch (c)
                    {
                        case '~':
                            buffer.push_back('~');
                            buffer.push_back('0');
                            break;
                        case '/':
                            buffer.push_back('~');
                            buffer.push_back('1');
                            break;
                        default:
                            buffer.push_back(c);
                            break;
                    }
                }
            }
            return buffer;
        }

        // Iterators
        iterator begin() const
        {
            return tokens_.begin();
        }
        iterator end() const
        {
            return tokens_.end();
        }

        reverse_iterator rbegin() const
        {
            return tokens_.rbegin();
        }
        reverse_iterator rend() const
        {
            return tokens_.rend();
        }

        // Non-member functions
        friend basic_json_pointer<CharT> operator/(const basic_json_pointer<CharT>& lhs, const string_type& rhs)
        {
            basic_json_pointer<CharT> p(lhs);
            p /= rhs;
            return p;
        }

        friend basic_json_pointer<CharT> operator+( const basic_json_pointer<CharT>& lhs, const basic_json_pointer<CharT>& rhs )
        {
            basic_json_pointer<CharT> p(lhs);
            p += rhs;
            return p;
        }

        friend bool operator==( const basic_json_pointer& lhs, const basic_json_pointer& rhs )
        {
            return lhs.tokens_ == rhs.tokens_;
        }

        friend bool operator!=( const basic_json_pointer& lhs, const basic_json_pointer& rhs )
        {
            return lhs.tokens_ != rhs.tokens_;
        }

        friend std::basic_ostream<CharT>&
        operator<<( std::basic_ostream<CharT>& os, const basic_json_pointer<CharT>& p )
        {
            os << p.to_string();
            return os;
        }
    };

    template <class CharT,class IntegerType>
    typename std::enable_if<traits_extension::is_integer<IntegerType>::value, basic_json_pointer<CharT>>::type
    operator/(const basic_json_pointer<CharT>& lhs, IntegerType rhs)
    {
        basic_json_pointer<CharT> p(lhs);
        p /= rhs;
        return p;
    }

    using json_pointer = basic_json_pointer<char>;
    using wjson_pointer = basic_json_pointer<wchar_t>;

    #if !defined(JSONCONS_NO_DEPRECATED)
    template<class CharT>
    using basic_address = basic_json_pointer<CharT>;
    template<class CharT>
    using basic_json_ptr = basic_json_pointer<CharT>;
    JSONCONS_DEPRECATED_MSG("Instead, use json_pointer") typedef json_pointer address;
    JSONCONS_DEPRECATED_MSG("Instead, use json_pointer") typedef json_pointer json_ptr;
    JSONCONS_DEPRECATED_MSG("Instead, use wjson_pointer") typedef json_pointer wjson_ptr;
    #endif

    namespace detail {

    template <class Json>
    const Json* resolve(const Json* current, const typename Json::string_view_type& buffer, std::error_code& ec)
    {
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return current;
            }
            std::size_t index{0};
            auto result = turbo::detail::to_integer_decimal(buffer.data(), buffer.length(), index);
            if (!result)
            {
                ec = jsonpointer_errc::invalid_index;
                return current;
            }
            if (index >= current->size())
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return current;
            }
            current = std::addressof(current->at(index));
        }
        else if (current->is_object())
        {
            if (!current->contains(buffer))
            {
                ec = jsonpointer_errc::key_not_found;
                return current;
            }
            current = std::addressof(current->at(buffer));
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return current;
        }
        return current;
    }

    template <class Json>
    Json* resolve(Json* current, const typename Json::string_view_type& buffer, bool create_if_missing, std::error_code& ec)
    {
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return current;
            }
            std::size_t index{0};
            auto result = turbo::detail::to_integer_decimal(buffer.data(), buffer.length(), index);
            if (!result)
            {
                ec = jsonpointer_errc::invalid_index;
                return current;
            }
            if (index >= current->size())
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return current;
            }
            current = std::addressof(current->at(index));
        }
        else if (current->is_object())
        {
            if (!current->contains(buffer))
            {
                if (create_if_missing)
                {
                    auto r = current->try_emplace(buffer, Json());
                    current = std::addressof(r.first->value());
                }
                else
                {
                    ec = jsonpointer_errc::key_not_found;
                    return current;
                }
            }
            else
            {
                current = std::addressof(current->at(buffer));
            }
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return current;
        }
        return current;
    }

    } // namespace detail

    // get

    template<class Json>
    Json& get(Json& root, 
              const basic_json_pointer<typename Json::char_type>& location, 
              bool create_if_missing,
              std::error_code& ec)
    {
        if (location.empty())
        {
            return root;
        }

        Json* current = std::addressof(root);
        auto it = location.begin();
        auto end = location.end();
        while (it != end)
        {
            current = turbo::jsonpointer::detail::resolve(current, *it, create_if_missing, ec);
            if (ec)
                return *current;
            ++it;
        }
        return *current;
    }

    template<class Json, class StringSource>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,Json&>::type
    get(Json& root, 
        const StringSource& location_str, 
        bool create_if_missing,
        std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (ec)
        {
            return root;
        }
        return get(root, jsonptr, create_if_missing, ec);
    }

    template<class Json>
    const Json& get(const Json& root, 
                    const basic_json_pointer<typename Json::char_type>& location, 
                    std::error_code& ec)
    {
        if (location.empty())
        {
            return root;
        }

        const Json* current = std::addressof(root);
        auto it = location.begin();
        auto end = location.end();
        while (it != end)
        {
            current = turbo::jsonpointer::detail::resolve(current, *it, ec);
            if (ec)
                return *current;
            ++it;
        }
        return *current;
    }

    template<class Json, class StringSource>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,const Json&>::type
    get(const Json& root, 
        const StringSource& location_str, 
        std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (ec)
        {
            return root;
        }
        return get(root, jsonptr, ec);
    }

    template<class Json>
    Json& get(Json& root, 
              const basic_json_pointer<typename Json::char_type>& location, 
              std::error_code& ec)
    {
        return get(root, location, false, ec);
    }

    template<class Json, class StringSource>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,Json&>::type
    get(Json& root, 
        const StringSource& location_str, 
        std::error_code& ec)
    {
        return get(root, location_str, false, ec);
    }

    template<class Json>
    Json& get(Json& root, 
              const basic_json_pointer<typename Json::char_type>& location,
              bool create_if_missing = false)
    {
        std::error_code ec;
        Json& j = get(root, location, create_if_missing, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
        return j;
    }

    template<class Json, class StringSource>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,Json&>::type
    get(Json& root, 
              const StringSource& location_str,
              bool create_if_missing = false)
    {
        std::error_code ec;
        Json& result = get(root, location_str, create_if_missing, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
        return result;
    }

    template<class Json>
    const Json& get(const Json& root, const basic_json_pointer<typename Json::char_type>& location)
    {
        std::error_code ec;
        const Json& j = get(root, location, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
        return j;
    }

    template<class Json, class StringSource>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,const Json&>::type
    get(const Json& root, const StringSource& location_str)
    {
        std::error_code ec;
        const Json& j = get(root, location_str, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
        return j;
    }

    // contains

    template<class Json>
    bool contains(const Json& root, const basic_json_pointer<typename Json::char_type>& location)
    {
        std::error_code ec;
        get(root, location, ec);
        return !ec ? true : false;
    }

    template<class Json, class StringSource>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,bool>::type
    contains(const Json& root, const StringSource& location_str)
    {
        std::error_code ec;
        get(root, location_str, ec);
        return !ec ? true : false;
    }

    template<class Json,class T>
    void add(Json& root, 
             const basic_json_pointer<typename Json::char_type>& location, 
             T&& value, 
             bool create_if_missing,
             std::error_code& ec)
    {
        Json* current = std::addressof(root);

        std::basic_string<typename Json::char_type> buffer;
        auto it = location.begin();
        auto end = location.end();
        while (it != end)
        {
            buffer = *it;
            ++it;
            if (it != end)
            {
                current = turbo::jsonpointer::detail::resolve(current, buffer, create_if_missing, ec);
                if (ec)
                    return;
            }
        }
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                current->emplace_back(std::forward<T>(value));
                current = std::addressof(current->at(current->size()-1));
            }
            else
            {
                std::size_t index{0};
                auto result = turbo::detail::to_integer_decimal(buffer.data(), buffer.length(), index);
                if (!result)
                {
                    ec = jsonpointer_errc::invalid_index;
                    return;
                }
                if (index > current->size())
                {
                    ec = jsonpointer_errc::index_exceeds_array_size;
                    return;
                }
                if (index == current->size())
                {
                    current->emplace_back(std::forward<T>(value));
                    current = std::addressof(current->at(current->size()-1));
                }
                else
                {
                    auto it2 = current->insert(current->array_range().begin()+index,std::forward<T>(value));
                    current = std::addressof(*it2);
                }
            }
        }
        else if (current->is_object())
        {
            auto r = current->insert_or_assign(buffer,std::forward<T>(value));
            current = std::addressof(r.first->value());
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return;
        }
    }

    // add
    template<class Json, class StringSource, class T>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    add(Json& root, 
             const StringSource& location_str, 
             T&& value, 
             bool create_if_missing,
             std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (ec)
        {
            return;
        }
        add(root, jsonptr, std::forward<T>(value), create_if_missing, ec);
    }

    template<class Json,class T>
    void add(Json& root, 
             const basic_json_pointer<typename Json::char_type>& location, 
             T&& value, 
             std::error_code& ec)
    {
        add(root, location, std::forward<T>(value), false, ec);
    }

    template<class Json, class StringSource, class T>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    add(Json& root, 
             const StringSource& location_str, 
             T&& value, 
             std::error_code& ec)
    {
        add(root, location_str, std::forward<T>(value), false, ec);
    }

    template<class Json,class T>
    void add(Json& root, 
             const basic_json_pointer<typename Json::char_type>& location, 
             T&& value,
             bool create_if_missing = false)
    {
        std::error_code ec;
        add(root, location, std::forward<T>(value), create_if_missing, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template<class Json, class StringSource, class T>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    add(Json& root, 
             const StringSource& location_str, 
             T&& value,
             bool create_if_missing = false)
    {
        std::error_code ec;
        add(root, location_str, std::forward<T>(value), create_if_missing, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    // add_if_absent

    template<class Json, class T>
    void add_if_absent(Json& root, 
                       const basic_json_pointer<typename Json::char_type>& location, 
                       T&& value, 
                       bool create_if_missing,
                       std::error_code& ec)
    {
        Json* current = std::addressof(root);

        std::basic_string<typename Json::char_type> buffer;
        auto it = location.begin();
        auto end = location.end();

        while (it != end)
        {
            buffer = *it;
            ++it;
            if (it != end)
            {
                current = turbo::jsonpointer::detail::resolve(current, buffer, create_if_missing, ec);
                if (ec)
                    return;
            }
        }
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                current->emplace_back(std::forward<T>(value));
                current = std::addressof(current->at(current->size()-1));
            }
            else
            {
                std::size_t index{0};
                auto result = turbo::detail::to_integer_decimal(buffer.data(), buffer.length(), index);
                if (!result)
                {
                    ec = jsonpointer_errc::invalid_index;
                    return;
                }
                if (index > current->size())
                {
                    ec = jsonpointer_errc::index_exceeds_array_size;
                    return;
                }
                if (index == current->size())
                {
                    current->emplace_back(std::forward<T>(value));
                    current = std::addressof(current->at(current->size()-1));
                }
                else
                {
                    auto it2 = current->insert(current->array_range().begin()+index,std::forward<T>(value));
                    current = std::addressof(*it2);
                }
            }
        }
        else if (current->is_object())
        {
            if (current->contains(buffer))
            {
                ec = jsonpointer_errc::key_already_exists;
                return;
            }
            else
            {
                auto r = current->try_emplace(buffer,std::forward<T>(value));
                current = std::addressof(r.first->value());
            }
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return;
        }
    }

    template<class Json, class StringSource, class T>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    add_if_absent(Json& root, 
                       const StringSource& location_str, 
                       T&& value, 
                       bool create_if_missing,
                       std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (ec)
        {
            return;
        }
        add_if_absent(root, jsonptr, std::forward<T>(value), create_if_missing, ec);
    }

    template<class Json, class StringSource, class T>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    add_if_absent(Json& root, 
                const StringSource& location, 
                T&& value, 
                std::error_code& ec)
    {
        add_if_absent(root, location, std::forward<T>(value), false, ec);
    }

    template<class Json, class StringSource, class T>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    add_if_absent(Json& root, 
                const StringSource& location_str, 
                T&& value,
                bool create_if_missing = false)
    {
        std::error_code ec;
        add_if_absent(root, location_str, std::forward<T>(value), create_if_missing, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template<class Json, class T>
    void add_if_absent(Json& root, 
                       const basic_json_pointer<typename Json::char_type>& location, 
                       T&& value, 
                       std::error_code& ec)
    {
        add_if_absent(root, location, std::forward<T>(value), false, ec);
    }

    template<class Json, class T>
    void add_if_absent(Json& root, 
                const basic_json_pointer<typename Json::char_type>& location, 
                T&& value,
                bool create_if_missing = false)
    {
        std::error_code ec;
        add_if_absent(root, location, std::forward<T>(value), create_if_missing, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    // remove

    template<class Json>
    void remove(Json& root, const basic_json_pointer<typename Json::char_type>& location, std::error_code& ec)
    {
        Json* current = std::addressof(root);

        std::basic_string<typename Json::char_type> buffer;
        auto it = location.begin();
        auto end = location.end();

        while (it != end)
        {
            buffer = *it;
            ++it;
            if (it != end)
            {
                current = turbo::jsonpointer::detail::resolve(current, buffer, false, ec);
                if (ec)
                    return;
            }
        }
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return;
            }
            else
            {
                std::size_t index{0};
                auto result = turbo::detail::to_integer_decimal(buffer.data(), buffer.length(), index);
                if (!result)
                {
                    ec = jsonpointer_errc::invalid_index;
                    return;
                }
                if (index >= current->size())
                {
                    ec = jsonpointer_errc::index_exceeds_array_size;
                    return;
                }
                current->erase(current->array_range().begin()+index);
            }
        }
        else if (current->is_object())
        {
            if (!current->contains(buffer))
            {
                ec = jsonpointer_errc::key_not_found;
                return;
            }
            else
            {
                current->erase(buffer);
            }
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return;
        }
    }

    template<class Json, class StringSource>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    remove(Json& root, const StringSource& location_str, std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (ec)
        {
            return;
        }
        remove(root, jsonptr, ec);
    }

    template<class Json, class StringSource>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    remove(Json& root, const StringSource& location_str)
    {
        std::error_code ec;
        remove(root, location_str, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template<class Json>
    void remove(Json& root, const basic_json_pointer<typename Json::char_type>& location)
    {
        std::error_code ec;
        remove(root, location, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    // replace

    template<class Json, class T>
    void replace(Json& root, 
                 const basic_json_pointer<typename Json::char_type>& location, 
                 T&& value, 
                 bool create_if_missing,
                 std::error_code& ec)
    {
        Json* current = std::addressof(root);

        std::basic_string<typename Json::char_type> buffer;
        auto it = location.begin();
        auto end = location.end();

        while (it != end)
        {
            buffer = *it;
            ++it;
            if (it != end)
            {
                current = turbo::jsonpointer::detail::resolve(current, buffer, create_if_missing, ec);
                if (ec)
                    return;
            }
        }
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return;
            }
            else
            {
                std::size_t index{};
                auto result = turbo::detail::to_integer_decimal(buffer.data(), buffer.length(), index);
                if (!result)
                {
                    ec = jsonpointer_errc::invalid_index;
                    return;
                }
                if (index >= current->size())
                {
                    ec = jsonpointer_errc::index_exceeds_array_size;
                    return;
                }
                current->at(index) = std::forward<T>(value);
            }
        }
        else if (current->is_object())
        {
            if (!current->contains(buffer))
            {
                if (create_if_missing)
                {
                    current->try_emplace(buffer,std::forward<T>(value));
                }
                else
                {
                    ec = jsonpointer_errc::key_not_found;
                    return;
                }
            }
            else
            {
                auto r = current->insert_or_assign(buffer,std::forward<T>(value));
                current = std::addressof(r.first->value());
            }
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return;
        }
    }

    template<class Json, class StringSource, class T>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    replace(Json& root, 
                 const StringSource& location_str, 
                 T&& value, 
                 bool create_if_missing,
                 std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (ec)
        {
            return;
        }
        replace(root, jsonptr, std::forward<T>(value), create_if_missing, ec);
    }

    template<class Json, class StringSource, class T>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    replace(Json& root, 
                 const StringSource& location_str, 
                 T&& value, 
                 std::error_code& ec)
    {
        replace(root, location_str, std::forward<T>(value), false, ec);
    }

    template<class Json, class StringSource, class T>
    typename std::enable_if<std::is_convertible<StringSource,turbo::basic_string_view<typename Json::char_type>>::value,void>::type
    replace(Json& root, 
                 const StringSource& location_str, 
                 T&& value, 
                 bool create_if_missing = false)
    {
        std::error_code ec;
        replace(root, location_str, std::forward<T>(value), create_if_missing, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template<class Json, class T>
    void replace(Json& root, 
                 const basic_json_pointer<typename Json::char_type>& location, 
                 T&& value, 
                 std::error_code& ec)
    {
        replace(root, location, std::forward<T>(value), false, ec);
    }

    template<class Json, class T>
    void replace(Json& root, 
                 const basic_json_pointer<typename Json::char_type>& location, 
                 T&& value, 
                 bool create_if_missing = false)
    {
        std::error_code ec;
        replace(root, location, std::forward<T>(value), create_if_missing, ec);
        if (ec)
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template <class String,class Result>
    typename std::enable_if<std::is_convertible<typename String::value_type,typename Result::value_type>::value>::type
    escape(const String& s, Result& result)
    {
        for (auto c : s)
        {
            if (c == '~')
            {
                result.push_back('~');
                result.push_back('0');
            }
            else if (c == '/')
            {
                result.push_back('~');
                result.push_back('1');
            }
            else
            {
                result.push_back(c);
            }
        }
    }

    template <class CharT>
    std::basic_string<CharT> escape(const turbo::basic_string_view<CharT>& s)
    {
        std::basic_string<CharT> result;

        for (auto c : s)
        {
            if (c == '~')
            {
                result.push_back('~');
                result.push_back('0');
            }
            else if (c == '/')
            {
                result.push_back('~');
                result.push_back('1');
            }
            else
            {
                result.push_back(c);
            }
        }
        return result;
    }

    // flatten

    template<class Json>
    void flatten_(const std::basic_string<typename Json::char_type>& parent_key,
                  const Json& parent_value,
                  Json& result)
    {
        using char_type = typename Json::char_type;
        using string_type = std::basic_string<char_type>;

        switch (parent_value.type())
        {
            case json_type::array_value:
            {
                if (parent_value.empty())
                {
                    // Flatten empty array to null
                    //result.try_emplace(parent_key, null_type{});
                    //result[parent_key] = parent_value;
                    result.try_emplace(parent_key, parent_value);
                }
                else
                {
                    for (std::size_t i = 0; i < parent_value.size(); ++i)
                    {
                        string_type key(parent_key);
                        key.push_back('/');
                        turbo::detail::from_integer(i,key);
                        flatten_(key, parent_value.at(i), result);
                    }
                }
                break;
            }

            case json_type::object_value:
            {
                if (parent_value.empty())
                {
                    // Flatten empty object to null
                    //result.try_emplace(parent_key, null_type{});
                    //result[parent_key] = parent_value;
                    result.try_emplace(parent_key, parent_value);
                }
                else
                {
                    for (const auto& item : parent_value.object_range())
                    {
                        string_type key(parent_key);
                        key.push_back('/');
                        escape(turbo::basic_string_view<char_type>(item.key().data(),item.key().size()), key);
                        flatten_(key, item.value(), result);
                    }
                }
                break;
            }

            default:
            {
                // add primitive parent_value with its reference string
                //result[parent_key] = parent_value;
                result.try_emplace(parent_key, parent_value);
                break;
            }
        }
    }

    template<class Json>
    Json flatten(const Json& value)
    {
        Json result;
        std::basic_string<typename Json::char_type> parent_key;
        flatten_(parent_key, value, result);
        return result;
    }


    // unflatten

    enum class unflatten_options {none,assume_object = 1
    #if !defined(JSONCONS_NO_DEPRECATED)
,object = assume_object
#endif
};

    template<class Json>
    Json safe_unflatten (Json& value)
    {
        if (!value.is_object() || value.empty())
        {
            return value;
        }
        bool safe = true;
        std::size_t index = 0;
        for (const auto& item : value.object_range())
        {
            std::size_t n;
            auto r = turbo::detail::to_integer_decimal(item.key().data(),item.key().size(), n);
            if (!r || (index++ != n))
            {
                safe = false;
                break;
            }
        }

        if (safe)
        {
            Json j(json_array_arg);
            j.reserve(value.size());
            for (auto& item : value.object_range())
            {
                j.emplace_back(std::move(item.value()));
            }
            Json a(json_array_arg);
            for (auto& item : j.array_range())
            {
                a.emplace_back(safe_unflatten (item));
            }
            return a;
        }
        else
        {
            Json o(json_object_arg);
            for (auto& item : value.object_range())
            {
                o.try_emplace(item.key(), safe_unflatten (item.value()));
            }
            return o;
        }
    }

    template<class Json>
    std::optional<Json> try_unflatten_array(const Json& value)
    {
        using char_type = typename Json::char_type;

        if (TURBO_UNLIKELY(!value.is_object()))
        {
            JSONCONS_THROW(jsonpointer_error(jsonpointer_errc::argument_to_unflatten_invalid));
        }
        Json result;

        for (const auto& item: value.object_range())
        {
            Json* part = &result;
            basic_json_pointer<char_type> ptr(item.key());
            std::size_t index = 0;
            for (auto it = ptr.begin(); it != ptr.end(); )
            {
                auto s = *it;
                size_t n{0};
                auto r = turbo::detail::to_integer_decimal(s.data(), s.size(), n);
                if (r.ec == turbo::detail::to_integer_errc() && (index++ == n))
                {
                    if (!part->is_array())
                    {
                        *part = Json(json_array_arg);
                    }
                    if (++it != ptr.end())
                    {
                        if (n+1 > part->size())
                        {
                            Json& ref = part->emplace_back();
                            part = std::addressof(ref);
                        }
                        else
                        {
                            part = &part->at(n);
                        }
                    }
                    else
                    {
                        Json& ref = part->emplace_back(item.value());
                        part = std::addressof(ref);
                    }
                }
                else if (part->is_object())
                {
                    if (++it != ptr.end())
                    {
                        auto res = part->try_emplace(s,Json());
                        part = &(res.first->value());
                    }
                    else
                    {
                        auto res = part->try_emplace(s, item.value());
                        part = &(res.first->value());
                    }
                }
                else 
                {
                    return std::optional<Json>();
                }
            }
        }

        return result;
    }

    template<class Json>
    Json unflatten_to_object(const Json& value, unflatten_options options = unflatten_options::none)
    {
        using char_type = typename Json::char_type;

        if (TURBO_UNLIKELY(!value.is_object()))
        {
            JSONCONS_THROW(jsonpointer_error(jsonpointer_errc::argument_to_unflatten_invalid));
        }
        Json result;

        for (const auto& item: value.object_range())
        {
            Json* part = &result;
            basic_json_pointer<char_type> ptr(item.key());
            for (auto it = ptr.begin(); it != ptr.end(); )
            {
                auto s = *it;
                if (++it != ptr.end())
                {
                    auto res = part->try_emplace(s,Json());
                    part = &(res.first->value());
                }
                else
                {
                    auto res = part->try_emplace(s, item.value());
                    part = &(res.first->value());
                }
            }
        }

        return options == unflatten_options::none ? safe_unflatten (result) : result;
    }

    template<class Json>
    Json unflatten(const Json& value, unflatten_options options = unflatten_options::none)
    {
        if (options == unflatten_options::none)
        {
            std::optional<Json> j = try_unflatten_array(value);
            return j ? *j : unflatten_to_object(value,options);
        }
        else
        {
            return unflatten_to_object(value,options);
        }
    }

#if !defined(JSONCONS_NO_DEPRECATED)

    template<class Json>
    JSONCONS_DEPRECATED_MSG("Instead, use add(Json&, const typename Json::string_view_type&, const Json&)")
    void insert_or_assign(Json& root, const std::basic_string<typename Json::char_type>& location, const Json& value)
    {
        add(root, location, value);
    }

    template<class Json>
    JSONCONS_DEPRECATED_MSG("Instead, use add(Json&, const typename Json::string_view_type&, const Json&, std::error_code&)")
    void insert_or_assign(Json& root, const std::basic_string<typename Json::char_type>& location, const Json& value, std::error_code& ec)
    {
        add(root, location, value, ec);
    }
    template<class Json, class T>
    void insert(Json& root, 
                const std::basic_string<typename Json::char_type>& location, 
                T&& value, 
                bool create_if_missing,
                std::error_code& ec)
    {
        add_if_absent(root,location,std::forward<T>(value),create_if_missing,ec);
    }

    template<class Json, class T>
    void insert(Json& root, 
                const std::basic_string<typename Json::char_type>& location, 
                T&& value, 
                std::error_code& ec)
    {
        add_if_absent(root, location, std::forward<T>(value), ec);
    }

    template<class Json, class T>
    void insert(Json& root, 
                const std::basic_string<typename Json::char_type>& location, 
                T&& value,
                bool create_if_missing = false)
    {
        add_if_absent(root, location, std::forward<T>(value), create_if_missing);
    }
#endif

} // namespace jsonpointer
} // namespace turbo

#endif
