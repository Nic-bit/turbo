// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/source_adaptor.h"
#include "catch/catch.hpp"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <new>
#include <fstream>

using namespace turbo;
 
TEST_CASE("buffer reader tests")
{
    SECTION("test 1")
    {
        json_source_adaptor<turbo::string_source<char>> reader{};
    }
}
 
TEST_CASE("json_source_adaptor constructor tests")
{
    SECTION("test 1")
    {
        json_source_adaptor<string_source<char>> source{string_source<char>()};
        CHECK(source.eof());
    }
}

