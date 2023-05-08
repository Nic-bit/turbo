﻿// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#if defined(_MSC_VER)
#include "windows.h" // test no inadvertant macro expansions
#endif
#include "catch/catch.hpp"
#include <iostream>
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/jsonpath/jsonpath.h"
#include "common/FreeListAllocator.h"

using namespace turbo;

#if !(defined(__GNUC__) && (__GNUC__ == 4)) || !(defined(__GNUC__) && __GNUC_MINOR__ < 9)

TEST_CASE("jsonpath stateful allocator test")
{
    using my_json = basic_json<char,sorted_policy,FreeListAllocator<char>>;
    std::string input = R"(
{ "store": {
    "book": [ 
      { "category": "reference",
        "author": "Nigel Rees",
        "title": "Sayings of the Century",
        "price": 8.95
      },
      { "category": "fiction",
        "author": "Evelyn Waugh",
        "title": "Sword of Honour",
        "price": 12.99
      },
      { "category": "fiction",
        "author": "Herman Melville",
        "title": "Moby Dick",
        "isbn": "0-553-21311-3",
        "price": 8.99
      }
    ]
  }
}
)";

    SECTION("make_expression")
    {
        json_decoder<my_json,FreeListAllocator<char>> decoder(result_allocator_arg, FreeListAllocator<char>(1),
                                                              FreeListAllocator<char>(2));

        auto myAlloc = FreeListAllocator<char>(3);        

        basic_json_reader<char,string_source<char>,FreeListAllocator<char>> reader(input, decoder, myAlloc);
        reader.read();

        my_json j = decoder.get_result();

        turbo::string_view p{"$..book[?(@.category == 'fiction')].title"};
        auto expr = turbo::jsonpath::make_expression<my_json>(std::allocator_arg, myAlloc, p);
        auto result = expr.evaluate(j);

        CHECK(result.size() == 2);
        CHECK(result[0].as_string_view() == "Sword of Honour");
        CHECK(result[1].as_string_view() == "Moby Dick");
    }
    SECTION("json_query 1")
    {
        json_decoder<my_json,FreeListAllocator<char>> decoder(result_allocator_arg, FreeListAllocator<char>(1),
                                                              FreeListAllocator<char>(2));

        auto myAlloc = FreeListAllocator<char>(3);        

        basic_json_reader<char,string_source<char>,FreeListAllocator<char>> reader(input, decoder, myAlloc);
        reader.read();

        my_json j = decoder.get_result();

        auto result = turbo::jsonpath::json_query(std::allocator_arg, myAlloc,
            j,"$..book[?(@.category == 'fiction')].title");

        CHECK(result.size() == 2);
        CHECK(result[0].as_string_view() == "Sword of Honour");
        CHECK(result[1].as_string_view() == "Moby Dick");
    }
    SECTION("json_query 2")
    {
        json_decoder<my_json,FreeListAllocator<char>> decoder(result_allocator_arg, FreeListAllocator<char>(1),
                                                              FreeListAllocator<char>(2));

        auto myAlloc = FreeListAllocator<char>(3);        

        basic_json_reader<char,string_source<char>,FreeListAllocator<char>> reader(input, decoder, myAlloc);
        reader.read();

        my_json j = decoder.get_result();

        jsonpath::json_query(std::allocator_arg, myAlloc, 
            j, "$..book[?(@.title == 'Sword of Honour')].title", 
            [](const turbo::string_view&, const my_json& title)
            {
                CHECK((title.as<turbo::string_view>() == "Sword of Honour"));
            }
        );
    }
    SECTION("json_replace 1")
    {
        json_decoder<my_json,FreeListAllocator<char>> decoder(result_allocator_arg, FreeListAllocator<char>(1),
                                                              FreeListAllocator<char>(2));

        auto myAlloc = FreeListAllocator<char>(3);        

        basic_json_reader<char,string_source<char>,FreeListAllocator<char>> reader(input, decoder, myAlloc);
        reader.read();

        my_json j = decoder.get_result();

        jsonpath::json_replace(std::allocator_arg, myAlloc,
            j,"$..book[?(@.price==12.99)].price", 30.9);

        CHECK(30.9 == Approx(j["store"]["book"][1]["price"].as<double>()).epsilon(0.001));
    }
    SECTION("json_replace 2")
    {
        json_decoder<my_json,FreeListAllocator<char>> decoder(result_allocator_arg, FreeListAllocator<char>(1),
                                                              FreeListAllocator<char>(2));

        auto myAlloc = FreeListAllocator<char>(3);        

        basic_json_reader<char,string_source<char>,FreeListAllocator<char>> reader(input, decoder, myAlloc);
        reader.read();

        my_json j = decoder.get_result();

        // make a discount on all books
        jsonpath::json_replace(std::allocator_arg, myAlloc,
            j, "$.store.book[*].price",
            [](const turbo::string_view&, my_json& price)
            {
                price = std::round(price.as<double>() - 1.0); 
            }
        );

        CHECK(8.0 == Approx(j["store"]["book"][0]["price"].as<double>()).epsilon(0.001));
        CHECK(12.0 == Approx(j["store"]["book"][1]["price"].as<double>()).epsilon(0.001));
        CHECK(8.0 == Approx(j["store"]["book"][2]["price"].as<double>()).epsilon(0.001));
    }
}

#endif
