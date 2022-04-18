#include <gtest/gtest.h>

#include <boost/type_traits/is_detected.hpp>

#include "hut/utils/length.hpp"

using namespace hut;

static_assert(std::is_trivial_v<length_px<f32>>);

static_assert(std::is_constructible_v<length_px<f32>, length_px<f32>>);
static_assert(std::is_constructible_v<length_px<f32>, length_px<f64>>);
static_assert(std::is_constructible_v<length_px<f32>, length_px<i32>>);
static_assert(std::is_constructible_v<length_px<f32>, length_px<u32>>);

static_assert(!std::is_constructible_v<length_px<f32>, length_pt<f32>>);
static_assert(!std::is_constructible_v<length_px<f32>, length_in<f32>>);
static_assert(!std::is_constructible_v<length_px<f32>, length_mm<f32>>);

static_assert(std::is_assignable_v<length_px<f32>, f32>);
static_assert(std::is_assignable_v<length_px<f32>, f64>);
static_assert(std::is_assignable_v<length_px<f32>, i32>);
static_assert(std::is_assignable_v<length_px<f32>, u32>);

static_assert(std::is_assignable_v<length_px<f32>, length_px<f32>>);
static_assert(std::is_assignable_v<length_px<f32>, length_px<f64>>);
static_assert(std::is_assignable_v<length_px<f32>, length_px<i32>>);
static_assert(std::is_assignable_v<length_px<f32>, length_px<u32>>);

static_assert(!std::is_assignable_v<length_px<f32>, length_pt<f32>>);
static_assert(!std::is_assignable_v<length_px<f32>, length_in<f32>>);
static_assert(!std::is_assignable_v<length_px<f32>, length_mm<f32>>);

template<typename TLeft, typename TRight> using can_eq = decltype( std::declval<TLeft>() == std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_ne = decltype( std::declval<TLeft>() != std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_gt = decltype( std::declval<TLeft>() > std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_lt = decltype( std::declval<TLeft>() < std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_ge = decltype( std::declval<TLeft>() >= std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_le = decltype( std::declval<TLeft>() <= std::declval<TRight>() );

#define HUT_CHECK_CMP(OP) \
  static_assert(boost::is_detected_v<OP, length_px<f32>, length_px<f32>>); \
  static_assert(boost::is_detected_v<OP, length_px<f32>, length_px<f64>>); \
  static_assert(boost::is_detected_v<OP, length_px<f32>, length_px<i32>>); \
  static_assert(boost::is_detected_v<OP, length_px<f32>, length_px<u32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, f32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, i32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, u32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_pt<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_in<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_mm<f32>>);

HUT_CHECK_CMP(can_eq)
HUT_CHECK_CMP(can_ne)
HUT_CHECK_CMP(can_gt)
HUT_CHECK_CMP(can_lt)
HUT_CHECK_CMP(can_ge)
HUT_CHECK_CMP(can_le)

template<typename TLeft, typename TRight> using can_add = decltype( std::declval<TLeft>() + std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_sub = decltype( std::declval<TLeft>() - std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_mul = decltype( std::declval<TLeft>() * std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_div = decltype( std::declval<TLeft>() / std::declval<TRight>() );

#define HUT_CHECK_ARITHMETICS_BINARY(OP) \
  static_assert(std::is_same_v<OP<length_px<f32>, length_px<f32>>, length_px<f32>>); \
  static_assert(std::is_same_v<OP<length_px<f32>, length_px<f64>>, length_px<f64>>); \
  static_assert(std::is_same_v<OP<length_px<f32>, length_px<i32>>, length_px<f32>>); \
  static_assert(std::is_same_v<OP<length_px<f32>, length_px<u32>>, length_px<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, f32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, i32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, u32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_pt<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_in<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_mm<f32>>);

HUT_CHECK_ARITHMETICS_BINARY(can_add)
HUT_CHECK_ARITHMETICS_BINARY(can_sub)

#define HUT_CHECK_ARITHMETICS_SCALAR(OP) \
  static_assert(std::is_same_v<OP<length_px<f32>, f32>, length_px<f32>>); \
  static_assert(std::is_same_v<OP<length_px<f32>, f64>, length_px<f64>>); \
  static_assert(std::is_same_v<OP<length_px<f32>, i32>, length_px<f32>>); \
  static_assert(std::is_same_v<OP<length_px<f32>, u32>, length_px<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_px<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_px<i32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_px<u32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_pt<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_in<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_mm<f32>>);

HUT_CHECK_ARITHMETICS_SCALAR(can_mul)
HUT_CHECK_ARITHMETICS_SCALAR(can_div)

template<typename TLeft, typename TRight> using can_addassign = decltype( std::declval<TLeft>() += std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_subassign = decltype( std::declval<TLeft>() -= std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_mulassign = decltype( std::declval<TLeft>() *= std::declval<TRight>() );
template<typename TLeft, typename TRight> using can_divassign = decltype( std::declval<TLeft>() /= std::declval<TRight>() );

#define HUT_CHECK_ASSIGN_BINARY(OP) \
  static_assert(boost::is_detected_v<OP, length_px<f32>, length_px<f32>>); \
  static_assert(boost::is_detected_v<OP, length_px<f32>, length_px<f64>>); \
  static_assert(boost::is_detected_v<OP, length_px<f32>, length_px<i32>>); \
  static_assert(boost::is_detected_v<OP, length_px<f32>, length_px<u32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, f32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, i32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, u32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_pt<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_in<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_mm<f32>>);

HUT_CHECK_ASSIGN_BINARY(can_addassign)
HUT_CHECK_ASSIGN_BINARY(can_subassign)

#define HUT_CHECK_ASSIGN_SCALAR(OP) \
  static_assert(boost::is_detected_v<OP, length_px<f32>, f32>); \
  static_assert(boost::is_detected_v<OP, length_px<f32>, f64>); \
  static_assert(boost::is_detected_v<OP, length_px<f32>, i32>); \
  static_assert(boost::is_detected_v<OP, length_px<f32>, u32>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_px<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_px<i32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_px<u32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_pt<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_in<f32>>); \
  static_assert(!boost::is_detected_v<OP, length_px<f32>, length_mm<f32>>);

HUT_CHECK_ASSIGN_SCALAR(can_mulassign)
HUT_CHECK_ASSIGN_SCALAR(can_divassign)

TEST(length, comparaisons) {
  EXPECT_EQ(length_px<u32>{0}, length_px<u64>{0});
  EXPECT_NE(length_px<u32>{0}, length_px<u64>{1});
  EXPECT_GT(length_px<u32>{1}, length_px<u64>{0});
  EXPECT_LT(length_px<u32>{0}, length_px<u64>{1});
  EXPECT_GE(length_px<u32>{1}, length_px<u64>{0});
  EXPECT_GE(length_px<u32>{1}, length_px<u64>{1});
  EXPECT_LE(length_px<u32>{0}, length_px<u64>{1});
  EXPECT_LE(length_px<u32>{1}, length_px<u64>{1});
}

TEST(length, arithmetics) {
  EXPECT_EQ(length_px<u32>{8}, length_px<u16>{6} + length_px<u32>{2});
  EXPECT_EQ(length_px<u32>{6}, length_px<u16>{8} - length_px<u32>{2});
  EXPECT_EQ(length_px<u32>{8}, length_px<u16>{4} * 2u);
  EXPECT_EQ(length_px<u32>{4}, length_px<u16>{8} / 2u);
}

TEST(length, modifiers) {
  length_px<u32> a;
  a = length_px<u32>{8};
  EXPECT_EQ(length_px<u32>{8}, a);
  a += length_px<u32>{8};
  EXPECT_EQ(length_px<u32>{16}, a);
  a -= length_px<u32>{8};
  EXPECT_EQ(length_px<u32>{8}, a);
  a *= 2;
  EXPECT_EQ(length_px<u32>{16}, a);
  a /= 2;
  EXPECT_EQ(length_px<u32>{8}, a);
}

TEST(length, literals) {
  EXPECT_EQ(length_px<u32>{10}, 10_px);
  EXPECT_EQ(length_pt<f32>{10}, 10._pt);
  EXPECT_EQ(length_in<f32>{10}, 10._in);
  EXPECT_EQ(length_mm<f32>{10}, 10._mm);
}

TEST(length, scale_pt) {
  EXPECT_EQ(length_px<u16>{100}, scale(length_pt<u16>{100}, 72));
  EXPECT_EQ(length_px<u16>{200}, scale(length_pt<u16>{200}, 72));
  EXPECT_EQ(length_px<u16>{200}, scale(length_pt<u16>{100}, 144));
  EXPECT_EQ(length_px<u16>{50}, scale(length_pt<u16>{100}, 36));
}

TEST(length, scale_in) {
  EXPECT_EQ(length_px<u16>{960}, scale(length_in<u16>{10}, 96));
  EXPECT_EQ(length_px<u16>{1920}, scale(length_in<u16>{20}, 96));
  EXPECT_EQ(length_px<u16>{1920}, scale(length_in<u16>{10}, 192));
  EXPECT_EQ(length_px<u16>{480}, scale(length_in<u16>{10}, 48));
}

TEST(length, scale_mm) {
  EXPECT_FLOAT_EQ((f32)length_px<f32>{96 / 25.4f * 10}, (f32)scale(length_mm<f32>{10}, 96));
}

static_assert(std::is_constructible_v<vec2_px, f32, f32>);
static_assert(std::is_constructible_v<vec2_px, length_px<f32>, f32>);
static_assert(std::is_constructible_v<vec2_px, f32, length_px<f32>>);
static_assert(std::is_constructible_v<vec2_px, length_px<f32>, length_px<f32>>);

// Accepted as params, but wouldn't compile cause length_pt can't be casted
//static_assert(!std::is_constructible_v<vec2_px, length_pt<f32>, f32>);
//static_assert(!std::is_constructible_v<vec2_px, f32, length_pt<f32>>);
//static_assert(!std::is_constructible_v<vec2_px, length_pt<f32>, length_pt<f32>>);

// Accepted as params, but wouldn't compile cause length_pt can't be casted
//static_assert(!std::is_constructible_v<vec2_px, vec2_pt>);

TEST(length, vec2) {
  EXPECT_NE(u16vec2_px(0_px, 0_px), u16vec2_px(0_px, 1_px));
  EXPECT_EQ(u16vec2_px(0, 0), u16vec2_px(0_px, 0_px));
  EXPECT_EQ(u16vec2_px(1, 1) + u16vec2_px(2, 2), u16vec2_px(3_px, 3_px));
  EXPECT_EQ(u16vec2_px(2, 2) * u16(3), u16vec2_px(6_px, 6_px));
  EXPECT_EQ(u16vec2_px(2, 2) / u16(2), u16vec2_px(1_px, 1_px));
}
