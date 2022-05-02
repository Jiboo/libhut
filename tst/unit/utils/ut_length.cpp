#include <boost/type_traits/is_detected.hpp>
#include <gtest/gtest.h>

#include "hut/utils/length.hpp"

using namespace hut;

static_assert(std::is_trivial_v<f32_px>);

static_assert(std::is_constructible_v<f32_px, f32_px>);
static_assert(std::is_constructible_v<f32_px, f64_px>);
static_assert(std::is_constructible_v<f32_px, i32_px>);
static_assert(std::is_constructible_v<f32_px, u32_px>);

static_assert(!std::is_constructible_v<f32_px, f32_pt>);
static_assert(!std::is_constructible_v<f32_px, f32_in>);
static_assert(!std::is_constructible_v<f32_px, f32_mm>);

static_assert(std::is_assignable_v<f32_px, f32>);
static_assert(std::is_assignable_v<f32_px, f64>);
static_assert(std::is_assignable_v<f32_px, i32>);
static_assert(std::is_assignable_v<f32_px, u32>);

static_assert(std::is_assignable_v<f32_px, f32_px>);
static_assert(std::is_assignable_v<f32_px, f64_px>);
static_assert(std::is_assignable_v<f32_px, i32_px>);
static_assert(std::is_assignable_v<f32_px, u32_px>);

static_assert(!std::is_assignable_v<f32_px, f32_pt>);
static_assert(!std::is_assignable_v<f32_px, f32_in>);
static_assert(!std::is_assignable_v<f32_px, f32_mm>);

template<typename TLeft, typename TRight>
using can_eq = decltype(std::declval<TLeft>() == std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_ne = decltype(std::declval<TLeft>() != std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_gt = decltype(std::declval<TLeft>() > std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_lt = decltype(std::declval<TLeft>() < std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_ge = decltype(std::declval<TLeft>() >= std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_le = decltype(std::declval<TLeft>() <= std::declval<TRight>());

#define HUT_CHECK_CMP(OP)                                                                                              \
  static_assert(boost::is_detected_v<OP, f32_px, f32_px>);                                                             \
  static_assert(boost::is_detected_v<OP, f32_px, f64_px>);                                                             \
  static_assert(boost::is_detected_v<OP, f32_px, i32_px>);                                                             \
  static_assert(boost::is_detected_v<OP, f32_px, u32_px>);                                                             \
  static_assert(!boost::is_detected_v<OP, f32_px, f32>);                                                               \
  static_assert(!boost::is_detected_v<OP, f32_px, i32>);                                                               \
  static_assert(!boost::is_detected_v<OP, f32_px, u32>);                                                               \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_pt>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_in>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_mm>);

HUT_CHECK_CMP(can_eq)
HUT_CHECK_CMP(can_ne)
HUT_CHECK_CMP(can_gt)
HUT_CHECK_CMP(can_lt)
HUT_CHECK_CMP(can_ge)
HUT_CHECK_CMP(can_le)

template<typename TLeft, typename TRight>
using can_add = decltype(std::declval<TLeft>() + std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_sub = decltype(std::declval<TLeft>() - std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_mul = decltype(std::declval<TLeft>() * std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_div = decltype(std::declval<TLeft>() / std::declval<TRight>());

#define HUT_CHECK_ARITHMETICS_BINARY(OP)                                                                               \
  static_assert(std::is_same_v<HUT_PACK(OP) < f32_px, f32_px>, f32_px >);                                              \
  static_assert(std::is_same_v<HUT_PACK(OP) < f32_px, f64_px>, f64_px >);                                              \
  static_assert(std::is_same_v<HUT_PACK(OP) < f32_px, i32_px>, f32_px >);                                              \
  static_assert(std::is_same_v<HUT_PACK(OP) < f32_px, u32_px>, f32_px >);                                              \
  static_assert(!boost::is_detected_v<OP, f32_px, f32>);                                                               \
  static_assert(!boost::is_detected_v<OP, f32_px, i32>);                                                               \
  static_assert(!boost::is_detected_v<OP, f32_px, u32>);                                                               \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_pt>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_in>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_mm>);

HUT_CHECK_ARITHMETICS_BINARY(can_add)
HUT_CHECK_ARITHMETICS_BINARY(can_sub)

#define HUT_CHECK_ARITHMETICS_SCALAR(OP)                                                                               \
  static_assert(std::is_same_v<HUT_PACK(OP) < f32_px, f32>, f32_px >);                                                 \
  static_assert(std::is_same_v<HUT_PACK(OP) < f32_px, f64>, f64_px >);                                                 \
  static_assert(std::is_same_v<HUT_PACK(OP) < f32_px, i32>, f32_px >);                                                 \
  static_assert(std::is_same_v<HUT_PACK(OP) < f32_px, u32>, f32_px >);                                                 \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_px>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, i32_px>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, u32_px>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_pt>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_in>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_mm>);

HUT_CHECK_ARITHMETICS_SCALAR(can_mul)
HUT_CHECK_ARITHMETICS_SCALAR(can_div)

template<typename TLeft, typename TRight>
using can_addassign = decltype(std::declval<TLeft>() += std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_subassign = decltype(std::declval<TLeft>() -= std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_mulassign = decltype(std::declval<TLeft>() *= std::declval<TRight>());
template<typename TLeft, typename TRight>
using can_divassign = decltype(std::declval<TLeft>() /= std::declval<TRight>());

#define HUT_CHECK_ASSIGN_BINARY(OP)                                                                                    \
  static_assert(boost::is_detected_v<OP, f32_px, f32_px>);                                                             \
  static_assert(boost::is_detected_v<OP, f32_px, f64_px>);                                                             \
  static_assert(boost::is_detected_v<OP, f32_px, i32_px>);                                                             \
  static_assert(boost::is_detected_v<OP, f32_px, u32_px>);                                                             \
  static_assert(!boost::is_detected_v<OP, f32_px, f32>);                                                               \
  static_assert(!boost::is_detected_v<OP, f32_px, i32>);                                                               \
  static_assert(!boost::is_detected_v<OP, f32_px, u32>);                                                               \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_pt>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_in>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_mm>);

HUT_CHECK_ASSIGN_BINARY(can_addassign)
HUT_CHECK_ASSIGN_BINARY(can_subassign)

#define HUT_CHECK_ASSIGN_SCALAR(OP)                                                                                    \
  static_assert(boost::is_detected_v<OP, f32_px, f32>);                                                                \
  static_assert(boost::is_detected_v<OP, f32_px, f64>);                                                                \
  static_assert(boost::is_detected_v<OP, f32_px, i32>);                                                                \
  static_assert(boost::is_detected_v<OP, f32_px, u32>);                                                                \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_px>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, i32_px>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, u32_px>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_pt>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_in>);                                                            \
  static_assert(!boost::is_detected_v<OP, f32_px, f32_mm>);

HUT_CHECK_ASSIGN_SCALAR(can_mulassign)
HUT_CHECK_ASSIGN_SCALAR(can_divassign)

TEST(length, comparaisons) {
  EXPECT_EQ(u32_px{0}, u64_px{0});
  EXPECT_NE(u32_px{0}, u64_px{1});
  EXPECT_GT(u32_px{1}, u64_px{0});
  EXPECT_LT(u32_px{0}, u64_px{1});
  EXPECT_GE(u32_px{1}, u64_px{0});
  EXPECT_GE(u32_px{1}, u64_px{1});
  EXPECT_LE(u32_px{0}, u64_px{1});
  EXPECT_LE(u32_px{1}, u64_px{1});
}

TEST(length, arithmetics) {
  EXPECT_EQ(u32_px{8}, u16_px{6} + u32_px{2});
  EXPECT_EQ(u32_px{6}, u16_px{8} - u32_px{2});
  EXPECT_EQ(u32_px{8}, u16_px{4} * 2u);
  EXPECT_EQ(u32_px{4}, u16_px{8} / 2u);
}

TEST(length, modifiers) {
  u32_px a;
  a = u32_px{8};
  EXPECT_EQ(u32_px{8}, a);
  a += u32_px{8};
  EXPECT_EQ(u32_px{16}, a);
  a -= u32_px{8};
  EXPECT_EQ(u32_px{8}, a);
  a *= 2;
  EXPECT_EQ(u32_px{16}, a);
  a /= 2;
  EXPECT_EQ(u32_px{8}, a);
}

TEST(length, literals) {
  EXPECT_EQ(u32_px{10}, 10_px);
  EXPECT_EQ(f32_pt{10}, 10._pt);
  EXPECT_EQ(f32_in{10}, 10._in);
  EXPECT_EQ(f32_mm{10}, 10._mm);
}

TEST(length, scale_pt) {
  EXPECT_EQ(u16_px{100}, scale(u16_pt{100}, 72));
  EXPECT_EQ(u16_px{200}, scale(u16_pt{200}, 72));
  EXPECT_EQ(u16_px{200}, scale(u16_pt{100}, 144));
  EXPECT_EQ(u16_px{50}, scale(u16_pt{100}, 36));
}

TEST(length, scale_in) {
  EXPECT_EQ(u16_px{960}, scale(u16_in{10}, 96));
  EXPECT_EQ(u16_px{1920}, scale(u16_in{20}, 96));
  EXPECT_EQ(u16_px{1920}, scale(u16_in{10}, 192));
  EXPECT_EQ(u16_px{480}, scale(u16_in{10}, 48));
}

TEST(length, scale_mm) {
  EXPECT_FLOAT_EQ((f32)f32_px{96 / 25.4f * 10}, (f32)scale(f32_mm{10}, 96));
}

static_assert(std::is_constructible_v<vec2_px, f32, f32>);
static_assert(std::is_constructible_v<vec2_px, f32_px, f32>);
static_assert(std::is_constructible_v<vec2_px, f32, f32_px>);
static_assert(std::is_constructible_v<vec2_px, f32_px, f32_px>);

// Accepted as params, but wouldn't compile cause length_pt can't be casted
//static_assert(!std::is_constructible_v<vec2_px, f32_pt, f32>);
//static_assert(!std::is_constructible_v<vec2_px, f32, f32_pt>);
//static_assert(!std::is_constructible_v<vec2_px, f32_pt, f32_pt>);

// Accepted as params, but wouldn't compile cause length_pt can't be casted
//static_assert(!std::is_constructible_v<vec2_px, vec2_pt>);

TEST(length, vec2) {
  EXPECT_NE(u16vec2_px(0_px, 0_px), u16vec2_px(0_px, 1_px));
  EXPECT_EQ(u16vec2_px(0, 0), u16vec2_px(0_px, 0_px));
  EXPECT_EQ(u16vec2_px(1, 1) + u16vec2_px(2, 2), u16vec2_px(3_px, 3_px));
  EXPECT_EQ(u16vec2_px(2, 2) * u16(3), u16vec2_px(6_px, 6_px));
  EXPECT_EQ(u16vec2_px(2, 2) / u16(2), u16vec2_px(1_px, 1_px));
}
