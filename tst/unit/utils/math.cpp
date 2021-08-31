#include <gtest/gtest.h>

#include "hut/utils/math.hpp"

using namespace hut;

TEST(utils, align) {
  EXPECT_EQ(4, align(2, 4));
  EXPECT_EQ(80, align(2, 80));
  EXPECT_EQ(160, align(82, 80));
}
