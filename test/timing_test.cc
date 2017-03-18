#include <gtest/gtest.h>
#include "timing.h"
#include "config.h"


/*int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}*/

TEST(ComparisionTest, LargeSmall) {
  Config config;
  Timing timing(config);
  EXPECT_EQ(timing.read_to_read_l, timing.read_to_read_s);

}