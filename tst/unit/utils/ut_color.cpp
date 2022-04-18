#include <gtest/gtest.h>

#include "hut/utils/color.hpp"

using namespace hut;

static_assert(std::is_trivial_v<u8vec4_rgba>);
static_assert(std::is_trivial_v<f32vec4_rgba>);

static_assert(std::is_constructible_v<u8vec4_rgba, f32vec4_rgba>);
static_assert(std::is_constructible_v<f32vec4_rgba, u8vec4_rgba>);

TEST(color, cast) {
  EXPECT_EQ(colors::CADET_BLUE, u8vec4_rgba{f32vec4_rgba{colors::CADET_BLUE}});
  EXPECT_EQ(f32vec4_rgba{colors::CADET_BLUE}, f32vec4_rgba{colors::CADET_BLUE});
}
