#include <gtest/gtest.h>

#include "hut_ktx2.hpp"
#include "unittests_ktx2.hpp"

#ifdef HUT_ENABLE_RENDERDOC
#include "hut_renderdoc.hpp"
#endif

TEST(ktx2, tex_rgba8888_ktx2) {
  hut::display d("ktx2");

  auto result = hut::ktx::load(d, hut::unittests_ktx2::tex_rgba8888_ktx2);
  d.flush_staged();

  ASSERT_TRUE(result.has_value());
  auto img = result.value();
  EXPECT_EQ(glm::u16vec2(256,256), img->size());
  EXPECT_EQ(1, img->layers());
  EXPECT_EQ(7, img->levels());
  EXPECT_EQ(VK_FORMAT_R8G8B8A8_SRGB, img->format());
}

TEST(ktx2, tex_bc1_ktx2) {
  hut::display d("ktx2");

/*#ifdef HUT_ENABLE_RENDERDOC
  hut::renderdoc::frame_begin();
#endif*/

  hut::ktx::load_params kparams;
  kparams.tiling_ = VK_IMAGE_TILING_OPTIMAL;
  auto result = hut::ktx::load(d, hut::unittests_ktx2::tex_bc1_ktx2, kparams);
  d.flush_staged();

  ASSERT_TRUE(result.has_value());
  auto img = result.value();
  EXPECT_EQ(glm::u16vec2(256,256), img->size());
  EXPECT_EQ(1, img->layers());
  EXPECT_EQ(7, img->levels());
  EXPECT_EQ(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, img->format());

/*#ifdef HUT_ENABLE_RENDERDOC
  hut::renderdoc::frame_end("tex_bc1_ktx2");
#endif*/
}

TEST(ktx2, tex_2layers_bc1_ktx2) {
  hut::display d("ktx2");
  hut::ktx::load_params kparams;
  kparams.tiling_ = VK_IMAGE_TILING_OPTIMAL;
  auto result = hut::ktx::load(d, hut::unittests_ktx2::tex_2layers_bc1_ktx2, kparams);
  d.flush_staged();

  ASSERT_TRUE(result.has_value());
  auto img = result.value();
  EXPECT_EQ(glm::u16vec2(256,256), img->size());
  EXPECT_EQ(2, img->layers());
  EXPECT_EQ(7, img->levels());
  EXPECT_EQ(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, img->format());
}

TEST(ktx2, tex_cubemap_bc1_ktx2) {
  hut::display d("ktx2");
  hut::ktx::load_params kparams;
  kparams.tiling_ = VK_IMAGE_TILING_OPTIMAL;
  auto result = hut::ktx::load(d, hut::unittests_ktx2::tex_cubemap_bc1_ktx2, kparams);
  d.flush_staged();

  ASSERT_TRUE(result.has_value());
  auto img = result.value();
  EXPECT_EQ(glm::u16vec2(256,256), img->size());
  EXPECT_EQ(6, img->layers());
  EXPECT_EQ(7, img->levels());
  EXPECT_EQ(VK_FORMAT_BC1_RGBA_SRGB_BLOCK, img->format());
}
