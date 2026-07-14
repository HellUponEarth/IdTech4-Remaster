#include <algorithm>
#include <string>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("legacy project paths can be normalized", "[build][paths]")
{
    std::string path = "neo\\game\\gamesys\\Class.cpp";
    std::replace(path.begin(), path.end(), '\\', '/');

    REQUIRE(path == "neo/game/gamesys/Class.cpp");
}

TEST_CASE("configured build supports byte-addressable storage", "[build][runtime]")
{
    REQUIRE(sizeof(char) == 1);
    REQUIRE(sizeof(unsigned char) == 1);
}
