#include <gtest/gtest.h>

#include "hut/utils/color.hpp"
#include "hut/utils/length.hpp"

#include "hut/offscreen.hpp"
#include "hut/pipeline.hpp"

#ifdef HUT_ENABLE_RENDERDOC
#  include "hut/renderdoc/renderdoc.hpp"
#endif

#include "tst_pipelines.hpp"

using namespace hut;

constexpr u8vec4_rgba D = {0, 0, 0, 0};          // Default pixel values after image creation
constexpr u8vec4_rgba C = {0, 0, 0, 255};        // Cleared for framebuffer
constexpr u8vec4_rgba W = {255, 255, 255, 255};  // Drawn
constexpr u8vec4_rgba I = {128, 128, 128, 128};  // Untouched

void dump(u8vec4_rgba *_data, u16vec4_px _box) {
  for (int x = int(_box.x); x < int(_box.z); x++) {
    for (int y = int(_box.y); y < int(_box.w); y++) {
      auto pixel = _data[int(_box.z) * y + x];
      if (pixel == D)
        std::cout << "D";
      else if (pixel == C)
        std::cout << "C";
      else if (pixel == W)
        std::cout << "W";
      else if (pixel == I)
        std::cout << "I";
      else
        std::cout << "?";
    }
    std::cout << std::endl;
  }
}

TEST(offscreen, display_noop) {
  display d("display_noop");
}

TEST(offscreen, buffer_noop) {
  display d("buffer_noop");
  auto    b = std::make_shared<buffer>(d, 1024 * 1024);
}

TEST(offscreen, image_noop) {
  display d("image_noop");

  image_params params;
  params.size_   = {4, 4};
  params.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  auto img       = std::make_shared<image>(d, params);
}

TEST(offscreen, offscreen_noop) {
  display d("offscreen_noop");

  image_params params;
  params.size_   = {4, 4};
  params.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  params.usage_ |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<image>(d, params);

  auto ofs = std::make_shared<offscreen>(img);
}

TEST(offscreen, offscreen_download) {
  display d("offscreen_download");

  auto buff = std::make_shared<buffer>(d, 1024 * 1024);

  image_params iparams;
  iparams.size_   = {4, 4};
  iparams.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  iparams.usage_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<image>(d, iparams);

  auto ofs = offscreen(img);

  d.flush_staged();  // staging has to be explicitly flushed in offscreen mode

  u8vec4_rgba pixel_data[5 * 5] = {
      // clang-format off
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      // clang-format on
  };
  ofs.download(std::span<u8>(&pixel_data[0].x, sizeof(pixel_data)), 5 * 4, image::subresource{{0, 0, 4, 4}});

  // 4*4 copy into a 5*5 buffer, extra pixel should be untouched, the rest is default initialized to transparent on image creation
  u8vec4_rgba pixel_ref[5 * 5] = {
      // clang-format off
      D, D, D, D, I,
      D, D, D, D, I,
      D, D, D, D, I,
      D, D, D, D, I,
      I, I, I, I, I,
      // clang-format on
  };

  dump(pixel_data, {0, 0, 5, 5});
  EXPECT_TRUE(std::equal(std::begin(pixel_ref), std::end(pixel_ref), std::begin(pixel_data)));
}

TEST(offscreen, offscreen_basic) {
  display d("offscreen_basic");

  auto buff = std::make_shared<buffer>(d, 1024 * 1024);

  image_params iparams;
  iparams.size_   = {4, 4};
  iparams.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  iparams.usage_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<image>(d, iparams);
  auto ofs = offscreen(img);

  auto rgb_pipeline = std::make_shared<pipeline_rgb>(ofs);
  auto indices      = buff->allocate<u16>(6);
  indices->set({0, 1, 2, 2, 1, 3});
  auto vertices = buff->allocate<pipeline_rgb::vertex>(4);
  vertices->set({
      pipeline_rgb::vertex{{0, 0}, {1, 1, 1}},
      pipeline_rgb::vertex{{0, 2}, {1, 1, 1}},
      pipeline_rgb::vertex{{2, 0}, {1, 1, 1}},
      pipeline_rgb::vertex{{2, 2}, {1, 1, 1}},
  });
  auto instances = buff->allocate<pipeline_rgb::instance>(2);
  instances->set({pipeline_rgb::instance{make_transform_mat4({0, 0}, {1, 1, 1})},
                  pipeline_rgb::instance{make_transform_mat4({2, 2}, {1, 1, 1})}});

  auto ubo = buff->allocate<proj_ubo>(1, d.ubo_align());
  ubo->set(proj_ubo{iparams.size_});
  rgb_pipeline->write(0, ubo);

  d.flush_staged();  // staging has to be explicitly flushed in offscreen mode

  ofs.draw([&](VkCommandBuffer _cb) { rgb_pipeline->draw(_cb, 0, indices, instances, vertices); });

  u8vec4_rgba pixel_data[4 * 4];
  ofs.download(std::span<u8>(&pixel_data[0].x, sizeof(pixel_data)), 4 * 4);

  u8vec4_rgba pixel_ref[4 * 4] = {
      // clang-format off
      W, W, C, C,
      W, W, C, C,
      C, C, W, W,
      C, C, W, W,
      // clang-format on
  };

  dump(pixel_data, {0, 0, 4, 4});
  EXPECT_TRUE(std::equal(std::begin(pixel_ref), std::end(pixel_ref), std::begin(pixel_data)));
}

TEST(offscreen, offscreen_download_offset) {
  display d("offscreen_download_offset");

  auto buff = std::make_shared<buffer>(d, 1024 * 1024);

  image_params iparams;
  iparams.size_   = {4, 4};
  iparams.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  iparams.usage_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<image>(d, iparams);

  {
    // Set pixel at [2,2]
    auto  updator = img->update();
    auto *ptr     = updator.data();
    memcpy(ptr + (2 * updator.staging_row_pitch() + 2 * 4), &W, 4);
  }

  auto ofs = offscreen(img);

  d.flush_staged();  // staging has to be explicitly flushed in offscreen mode

  u8vec4_rgba pixel_data[5 * 5] = {
      // clang-format off
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      // clang-format on
  };
  ofs.download(std::span<u8>(&pixel_data[0].x, sizeof(pixel_data)), 5 * 4, image::subresource{{2, 2, 4, 4}});

  u8vec4_rgba pixel_ref[5 * 5] = {
      // clang-format off
      W, D, I, I, I,
      D, D, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      // clang-format on
  };

  dump(pixel_data, {0, 0, 5, 5});
  EXPECT_TRUE(std::equal(std::begin(pixel_ref), std::end(pixel_ref), std::begin(pixel_data)));
}

TEST(offscreen, offscreen_render_offset) {
  display d("offscreen_render_offset");

#ifdef HUT_ENABLE_RENDERDOC
  renderdoc::frame_begin();
#endif

  auto buff = std::make_shared<buffer>(d, 1024 * 1024);

  image_params iparams;
  iparams.size_   = {4, 4};
  iparams.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  iparams.usage_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<image>(d, iparams);

  offscreen_params oparams;
  oparams.subres_.coords_ = make_bbox_with_origin_size(u16vec2{1, 1}, {2, 2});
  auto ofs                = offscreen(img, oparams);

  auto rgb_pipeline = std::make_shared<pipeline_rgb>(ofs);
  auto indices      = buff->allocate<u16>(6);
  indices->set({0, 1, 2, 2, 1, 3});
  auto vertices = buff->allocate<pipeline_rgb::vertex>(4);
  vertices->set({
      pipeline_rgb::vertex{{0, 0}, {1, 1, 1}},
      pipeline_rgb::vertex{{0, 1}, {1, 1, 1}},
      pipeline_rgb::vertex{{1, 0}, {1, 1, 1}},
      pipeline_rgb::vertex{{1, 1}, {1, 1, 1}},
  });
  auto instances = buff->allocate<pipeline_rgb::instance>(1);
  instances->set({
      pipeline_rgb::instance{make_transform_mat4({1, 1}, {1, 1, 1})},
  });

  auto ubo = buff->allocate<proj_ubo>(1, d.ubo_align());
  ubo->set(proj_ubo{iparams.size_});
  rgb_pipeline->write(0, ubo);

  d.flush_staged();  // staging has to be explicitly flushed in offscreen mode

  ofs.draw([&](VkCommandBuffer _cb) { rgb_pipeline->draw(_cb, 0, indices, instances, vertices); });

  u8vec4_rgba pixel_data[4 * 4] = {
      // clang-format off
      I, I, I, I,
      I, I, I, I,
      I, I, I, I,
      I, I, I, I,
      // clang-format on
  };
  ofs.download(std::span<u8>(&pixel_data[0].x, sizeof(pixel_data)), 4 * 4, image::subresource{{0, 0, 4, 4}});

  // Draw one white pixel at [2,2] (W), rest of the 3*3 offscreen should have been cleared (C), rest of the 4*4 image should be untouched,
  u8vec4_rgba pixel_ref[4 * 4] = {
      // clang-format off
      D, D, D, D,
      D, W, C, D,
      D, C, C, D,
      D, D, D, D,
      // clang-format on
  };

  dump(pixel_data, {0, 0, 4, 4});
  EXPECT_TRUE(std::equal(std::begin(pixel_ref), std::end(pixel_ref), std::begin(pixel_data)));

#ifdef HUT_ENABLE_RENDERDOC
  renderdoc::frame_end("offscreen_render_offset");
#endif
}
