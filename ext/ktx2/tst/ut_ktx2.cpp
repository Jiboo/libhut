#include <gtest/gtest.h>

#include "hut/ktx2/ktx2.hpp"

#include "tst_data.hpp"

#ifdef HUT_ENABLE_RENDERDOC
#  include "hut/renderdoc/renderdoc.hpp"
#endif

using namespace hut;

TEST(ktx2, tex_rgba8888_ktx2) {
  display       d("ktx2");
  shared_buffer b = std::make_shared<buffer>(d);

  auto result = ktx::load(d, b, tst_data::tex_rgba8888_ktx2);
  d.flush_staged();

  ASSERT_TRUE(result.has_value());
  auto img = result.value();
  EXPECT_EQ(u16vec2_px(256, 256), img->size());
  EXPECT_EQ(1, img->layers());
  EXPECT_EQ(7, img->levels());
  EXPECT_EQ(VK_FORMAT_R8G8B8A8_SRGB, img->format());
}

TEST(ktx2, tex_bc1_ktx2) {
  display       d("ktx2");
  shared_buffer b = std::make_shared<buffer>(d);

#ifdef HUT_ENABLE_RENDERDOC
  renderdoc::frame_begin();
#endif

  ktx::load_params kparams;
  kparams.tiling_ = VK_IMAGE_TILING_OPTIMAL;
  auto result     = ktx::load(d, b, tst_data::tex_bc1_ktx2, kparams);
  d.flush_staged();

  ASSERT_TRUE(result.has_value());
  auto img = result.value();
  EXPECT_EQ(u16vec2_px(256, 256), img->size());
  EXPECT_EQ(1, img->layers());
  EXPECT_EQ(7, img->levels());
  EXPECT_EQ(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, img->format());

#ifdef HUT_ENABLE_RENDERDOC
  renderdoc::frame_end("tex_bc1_ktx2");
#endif
}

TEST(ktx2, tex_2layers_bc1_ktx2) {
  display       d("ktx2");
  shared_buffer b = std::make_shared<buffer>(d);

  ktx::load_params kparams;
  kparams.tiling_ = VK_IMAGE_TILING_OPTIMAL;
  auto result     = ktx::load(d, b, tst_data::tex_2layers_bc1_ktx2, kparams);
  d.flush_staged();

  ASSERT_TRUE(result.has_value());
  auto img = result.value();
  EXPECT_EQ(u16vec2_px(256, 256), img->size());
  EXPECT_EQ(2, img->layers());
  EXPECT_EQ(7, img->levels());
  EXPECT_EQ(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, img->format());
}

TEST(ktx2, tex_cubemap_bc1_ktx2) {
  display       d("ktx2");
  shared_buffer b = std::make_shared<buffer>(d);

  ktx::load_params kparams;
  kparams.tiling_ = VK_IMAGE_TILING_OPTIMAL;
  auto result     = ktx::load(d, b, tst_data::tex_cubemap_bc1_ktx2, kparams);
  d.flush_staged();

  ASSERT_TRUE(result.has_value());
  auto img = result.value();
  EXPECT_EQ(u16vec2_px(256, 256), img->size());
  EXPECT_EQ(6, img->layers());
  EXPECT_EQ(7, img->levels());
  EXPECT_EQ(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, img->format());
}
