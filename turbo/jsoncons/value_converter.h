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

#ifndef JSONCONS_CONVERTER_HPP
#define JSONCONS_CONVERTER_HPP

#include <system_error> // std::error_code
#include "turbo/jsoncons/traits_extension.h"
#include "turbo/jsoncons/byte_string.h"
#include "turbo/jsoncons/json_type.h"
#include "turbo/jsoncons/conv_error.h"
#include "turbo/jsoncons/detail/write_number.h" // from_integer
#include "turbo/jsoncons/tag_type.h"

namespace turbo {

    template <class From, class Into, class Enable = void>
    class value_converter
    {
    };

    template <class Into>
    class value_converter_base
    {
    public:
        using allocator_type = typename std::conditional<traits_extension::has_allocator_type<Into>::value, typename Into::allocator_type, std::allocator<char>>::type;
    private:
        allocator_type alloc_;

    public:
        value_converter_base(const allocator_type& alloc = allocator_type())
            : alloc_(alloc)
        {
        }

        allocator_type get_allocator() const noexcept
        {
            return alloc_;
        }
    };

    // From any byte sequence, Into string
    template <class From, class Into>
    class value_converter<From, Into, 
        typename std::enable_if<traits_extension::is_byte_sequence<From>::value && !traits_extension::is_string_or_string_view<From>::value &&
            traits_extension::is_string<Into>::value>::type> : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;

        template <class CharT = typename Into::value_type>
        typename std::enable_if<traits_extension::is_narrow_character<CharT>::value,Into>::type
        convert(const From& value, semantic_tag tag, std::error_code&)
        {
            Into s(this->get_allocator());
            switch (tag)
            {
                case semantic_tag::base64:
                    encode_base64(value.begin(), value.end(), s);
                    break;
                case semantic_tag::base16:
                    encode_base16(value.begin(), value.end(), s);
                    break;
                default:
                    encode_base64url(value.begin(), value.end(), s);
                    break;
            }
            return s;
        }
        template <class CharT = typename Into::value_type>
        typename std::enable_if<traits_extension::is_wide_character<CharT>::value,Into>::type
        convert(const From& value, semantic_tag tag, std::error_code& ec)
        {
            std::string s;
            switch (tag)
            {
                case semantic_tag::base64:
                    encode_base64(value.begin(), value.end(), s);
                    break;
                case semantic_tag::base16:
                    encode_base16(value.begin(), value.end(), s);
                    break;
                default:
                    encode_base64url(value.begin(), value.end(), s);
                    break;
            }

            Into ws(this->get_allocator());
            auto retval = unicode_traits::convert(s.data(), s.size(), ws);
            if (retval.ec != unicode_traits::conv_errc())
            {
                ec = conv_errc::not_wide_char;
            }

            return ws;
        }
    };

    // From byte string, Into byte string
    template <class From, class Into>
    class value_converter<From, Into, 
        typename std::enable_if<traits_extension::is_byte_sequence<From>::value && 
            !traits_extension::is_string_or_string_view<From>::value &&
            !traits_extension::is_string_or_string_view<Into>::value && 
            traits_extension::is_back_insertable_byte_container<Into>::value>::type> : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;

        Into convert(const From& value, semantic_tag, std::error_code&)
        {
            Into s(value.begin(),value.end(),this->get_allocator());
            return s;
        }
    };
    
    // From string or string_view, Into string, same character type
    template <class From, class Into>
    class value_converter<From, Into, 
        typename std::enable_if<traits_extension::is_string_or_string_view<From>::value &&
            traits_extension::is_string<Into>::value && 
            std::is_same<typename From::value_type,typename Into::value_type>::value>::type> : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;

        Into convert(const From& value, semantic_tag, std::error_code&)
        {
            return Into(value.begin(),value.end(),this->get_allocator());
        }
    };

    // From string or string_view, Into string, different character type
    template <class From, class Into>
    class value_converter<From, Into, 
        typename std::enable_if<traits_extension::is_string_or_string_view<From>::value &&
            traits_extension::is_string<Into>::value && 
            !std::is_same<typename From::value_type,typename Into::value_type>::value>::type> : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;

        Into convert(const From& value, semantic_tag, std::error_code& ec)
        {
            Into ws(this->get_allocator());
            auto retval = unicode_traits::convert(value.data(), value.size(), ws);
            if (retval.ec != unicode_traits::conv_errc())
            {
                ec = conv_errc::not_wide_char;
            }

            return ws;
        }
    };

    // From string, Into byte_string
    template <class From, class Into>
    class value_converter<From, Into, 
        typename std::enable_if<traits_extension::is_char_sequence<From>::value &&
            !traits_extension::is_string_or_string_view<Into>::value && 
            traits_extension::is_back_insertable_byte_container<Into>::value>::type> : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;

        template <class CharT = typename From::value_type>
        typename std::enable_if<traits_extension::is_narrow_character<CharT>::value,Into>::type
        convert(const From& value, semantic_tag tag, std::error_code& ec)
        {
            Into bytes(this->get_allocator());
            switch (tag)
            {
                case semantic_tag::base16:
                {
                    auto res = decode_base16(value.begin(), value.end(), bytes);
                    if (res.ec != conv_errc::success)
                    {
                        ec = conv_errc::not_byte_string;
                    }
                    break;
                }
                case semantic_tag::base64:
                {
                    decode_base64(value.begin(), value.end(), bytes);
                    break;
                }
                case semantic_tag::base64url:
                {
                    decode_base64url(value.begin(), value.end(), bytes);
                    break;
                }
                default:
                {
                    ec = conv_errc::not_byte_string;
                    break;
                }
            }
            return bytes;
        }

        template <class CharT = typename From::value_type>
        typename std::enable_if<traits_extension::is_wide_character<CharT>::value,Into>::type
        convert(const From& value, semantic_tag tag, std::error_code& ec)
        {
            Into bytes(this->get_allocator());

            std::string s(this->get_allocator());
            auto retval = unicode_traits::convert(value.data(), value.size(), s);
            if (retval.ec != unicode_traits::conv_errc())
            {
                ec = conv_errc::not_wide_char;
            }
            switch (tag)
            {
                case semantic_tag::base16:
                {
                    auto res = decode_base16(s.begin(), s.end(), bytes);
                    if (res.ec != conv_errc::success)
                    {
                        ec = conv_errc::not_byte_string;
                    }
                    break;
                }
                case semantic_tag::base64:
                {
                    decode_base64(s.begin(), s.end(), bytes);
                    break;
                }
                case semantic_tag::base64url:
                {
                    decode_base64url(s.begin(), s.end(), bytes);
                    break;
                }
                default:
                {
                    ec = conv_errc::not_byte_string;
                    break;
                }
            }
            return bytes;
        }
    };

    // From integer, Into string
    template <class From, class Into>
    class value_converter<From, Into, 
        typename std::enable_if<traits_extension::is_integer<From>::value &&
            traits_extension::is_string<Into>::value>::type> : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;

        Into convert(From value, semantic_tag, std::error_code&)
        {
            Into s(this->get_allocator());
            turbo::detail::from_integer(value, s);
            return s;
        }
    };

    // From integer, Into string
    template <class From, class Into>
    class value_converter<From, Into, 
        typename std::enable_if<std::is_floating_point<From>::value &&
            traits_extension::is_string<Into>::value>::type> : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;

        Into convert(From value, semantic_tag, std::error_code&)
        {
            Into s(this->get_allocator());
            turbo::detail::write_double f{float_chars_format::general,0};
            f(value, s);
            return s;
        }
    };

    // From half, Into string
    template <class Into>
    class value_converter<half_arg_t, Into,
        typename std::enable_if<traits_extension::is_string<Into>::value>::type> : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;

        Into convert(uint16_t value, semantic_tag, std::error_code&)
        {
            Into s(this->get_allocator());
            turbo::detail::write_double f{float_chars_format::general,0};
            double x = binary::decode_half(value);
            f(x, s);
            return s;
        }
    };

    // From bool, Into string
    template <class From, class Into>
    class value_converter<From, Into, 
        typename std::enable_if<traits_extension::is_bool<From>::value &&
            traits_extension::is_string<Into>::value>::type> : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;
        using char_type = typename Into::value_type;

        constexpr
        Into convert(From value, semantic_tag, std::error_code&)
        {
            constexpr const char_type* true_constant = JSONCONS_CSTRING_CONSTANT(char_type,"true"); 
            constexpr const char_type* false_constant = JSONCONS_CSTRING_CONSTANT(char_type,"false"); 

            return value ? Into(true_constant,4) : Into(false_constant,5);
        }
    };

    // From null, Into string
    template <class Into>
    class value_converter<null_type, Into, void>  : value_converter_base<Into>
    {
    public:
        using allocator_type = typename value_converter_base<Into>::allocator_type;
        using char_type = typename Into::value_type;

        constexpr
        Into convert(semantic_tag, std::error_code&)
        {
            constexpr const char_type* null_constant = JSONCONS_CSTRING_CONSTANT(char_type,"null"); 

            return Into(null_constant,4);
        }
    };

} // namespace turbo

#endif

