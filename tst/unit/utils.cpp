#include <gtest/gtest.h>

#include "hut/utils.hpp"

TEST(utils, event) {
  hut::event<int> e1;
  e1.connect([](int a) -> bool { return a == 42; });
  e1.connect([](int a) -> bool { return a == 1337; });

  EXPECT_EQ(e1.fire(0), false);
  EXPECT_EQ(e1.fire(42), true);
  EXPECT_EQ(e1.fire(1337), true);
}

TEST(utils, align) {
  EXPECT_EQ(4, hut::align(2, 4));
  EXPECT_EQ(80, hut::align(2, 80));
  EXPECT_EQ(160, hut::align(82, 80));
}
