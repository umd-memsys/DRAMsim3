#include "catch.hpp"
#include "timing.h"

TEST_CASE("Timing Relations", "[timing]") {
    Config config;
    Timing timing(config);
    REQUIRE(timing.read_to_read_l == timing.read_to_read_s);
}