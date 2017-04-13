#include "catch.hpp"
#include "timing.h"

TEST_CASE("Timing Relations", "[timing]") {
    dramcore::Config config;
    dramcore::Timing timing(config);
    REQUIRE(timing.read_to_read_l == timing.read_to_read_s);
}