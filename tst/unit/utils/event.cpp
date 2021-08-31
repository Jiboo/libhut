#include <gtest/gtest.h>

#include "hut/utils/event.hpp"

using namespace hut;

TEST(utils, event) {
  event<int> e1;
  e1.connect([](int a) -> bool { return a == 42; });
  e1.connect([](int a) -> bool { return a == 1337; });

  EXPECT_EQ(e1.fire(0), false);
  EXPECT_EQ(e1.fire(42), true);
  EXPECT_EQ(e1.fire(1337), true);
}
