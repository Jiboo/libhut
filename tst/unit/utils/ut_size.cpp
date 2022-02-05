#include <gtest/gtest.h>

#include "hut/utils/size.hpp"

using namespace hut;

template<typename T>
using box_tst = boxed_number<T, "tst">;

TEST(size, cast) {
  box_tst<u32> t{0};
  box_tst<u64> t1 = t;
  box_tst<f64> t2 = t;
  box_tst<u16> t3 = t;
}

TEST(size, comparaisons) {
  EXPECT_EQ(box_tst<u32>{0}, box_tst<u64>{0});
  EXPECT_NE(box_tst<u32>{0}, box_tst<u64>{1});
  EXPECT_GT(box_tst<u32>{1}, box_tst<u64>{0});
  EXPECT_LT(box_tst<u32>{0}, box_tst<u64>{1});
  EXPECT_GE(box_tst<u32>{1}, box_tst<u64>{0});
  EXPECT_GE(box_tst<u32>{1}, box_tst<u64>{1});
  EXPECT_LE(box_tst<u32>{0}, box_tst<u64>{1});
  EXPECT_LE(box_tst<u32>{1}, box_tst<u64>{1});
}

TEST(size, arithmetics) {
  EXPECT_EQ(box_tst<u32>{8}, box_tst<u16>{0} + box_tst<u32>{8});
  EXPECT_EQ(box_tst<u32>{8}, box_tst<u16>{8} - box_tst<u32>{0});
  EXPECT_EQ(box_tst<u32>{8}, box_tst<u16>{4} * 2u);
  EXPECT_EQ(box_tst<u32>{4}, box_tst<u16>{8} / 2u);
}

TEST(size, modifiers) {
  box_tst<u32> a;
  a = box_tst<u32>{8};
  EXPECT_EQ(box_tst<u32>{8}, a);
  a += box_tst<u32>{8};
  EXPECT_EQ(box_tst<u32>{16}, a);
  a -= box_tst<u32>{8};
  EXPECT_EQ(box_tst<u32>{8}, a);
  a *= 2;
  EXPECT_EQ(box_tst<u32>{16}, a);
  a /= 2;
  EXPECT_EQ(box_tst<u32>{8}, a);
}

TEST(size, pt) {
  EXPECT_EQ(size_px<u16>{100}, size_pt<u16>{100}.scale(72));
  EXPECT_EQ(size_px<u16>{200}, size_pt<u16>{200}.scale(72));
  EXPECT_EQ(size_px<u16>{200}, size_pt<u16>{100}.scale(144));
  EXPECT_EQ(size_px<u16>{50}, size_pt<u16>{100}.scale(36));
}

TEST(size, in) {
  EXPECT_EQ(size_px<u16>{960}, size_in<u16>{10}.scale(96));
  EXPECT_EQ(size_px<u16>{1920}, size_in<u16>{20}.scale(96));
  EXPECT_EQ(size_px<u16>{1920}, size_in<u16>{10}.scale(192));
  EXPECT_EQ(size_px<u16>{480}, size_in<u16>{10}.scale(48));
}

TEST(size, mm) {
  EXPECT_FLOAT_EQ(size_px<f32>{96 / 25.4f * 10}.value_, size_mm<f32>{10}.scale(96).value_);
}

TEST(size, literals) {
  EXPECT_EQ(size_px<u32>{10}, 10_px);
  EXPECT_EQ(size_pt<f32>{10}, 10._pt);
  EXPECT_EQ(size_in<f32>{10}, 10._in);
  EXPECT_EQ(size_mm<f32>{10}, 10._mm);
}
