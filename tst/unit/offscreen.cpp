#include <gtest/gtest.h>

#include "hut/offscreen.hpp"
#include "hut/pipeline.hpp"

#ifdef HUT_ENABLE_RENDERDOC
#include "hut_renderdoc.hpp"
#endif

constexpr glm::u8vec4 D = {0, 0, 0, 0}; // Default pixel values after image creation
constexpr glm::u8vec4 C = {0, 0, 0, 255}; // Cleared for framebuffer
constexpr glm::u8vec4 W = {255, 255, 255, 255}; // Drawn
constexpr glm::u8vec4 I = {128, 128, 128, 128}; // Untouched

void dump(glm::u8vec4 *_data, glm::u16vec4 _box) {
  for (int x = _box.x; x < _box.z; x++) {
    for (int y = _box.y; y < _box.w; y++) {
      auto pixel = _data[_box.z * y + x];
      if (pixel == D) std::cout << "D";
      else if (pixel == C) std::cout << "C";
      else if (pixel == W) std::cout << "W";
      else if (pixel == I) std::cout << "I";
      else std::cout << "?";
    }
    std::cout << std::endl;
  }
}

TEST(offscreen, display_noop) {
  hut::display d("display_noop");
}

TEST(offscreen, buffer_noop) {
  hut::display d("buffer_noop");
  auto b = d.alloc_buffer(1024*1024);
}

TEST(offscreen, image_noop) {
  hut::display d("image_noop");

  hut::image_params params;
  params.size_ = {4, 4};
  params.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  auto img = std::make_shared<hut::image>(d, params);
}

TEST(offscreen, offscreen_noop) {
  hut::display d("offscreen_noop");

  hut::image_params params;
  params.size_ = {4, 4};
  params.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  params.usage_ |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<hut::image>(d, params);

  auto ofs = std::make_shared<hut::offscreen>(img);
}

TEST(offscreen, offscreen_download) {
  hut::display d("offscreen_download");

  auto buff = d.alloc_buffer(1024*1024);

  hut::image_params iparams;
  iparams.size_ = {4, 4};
  iparams.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  iparams.usage_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<hut::image>(d, iparams);

  auto ofs = hut::offscreen(img);

  d.flush_staged(); // staging has to be explicitly flushed in offscreen mode

  glm::u8vec4 pixel_data[5 * 5] = {
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
  };
  ofs.download(std::span<glm::u8>(&pixel_data[0].x, sizeof(pixel_data)), 5*4, hut::image::subresource{{0,0,4,4}});

  // 4*4 copy into a 5*5 buffer, extra pixel should be untouched, the rest is default initialized to transparent on image creation
  glm::u8vec4 pixel_ref[5 * 5] = {
      D, D, D, D, I,
      D, D, D, D, I,
      D, D, D, D, I,
      D, D, D, D, I,
      I, I, I, I, I,
  };

  dump(pixel_data, {0, 0, 5, 5});
  EXPECT_TRUE(std::equal(std::begin(pixel_ref), std::end(pixel_ref), std::begin(pixel_data)));
}

TEST(offscreen, offscreen_basic) {
  hut::display d("offscreen_basic");

  auto buff = d.alloc_buffer(1024*1024);

  hut::image_params iparams;
  iparams.size_ = {4, 4};
  iparams.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  iparams.usage_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<hut::image>(d, iparams);
  auto ofs = hut::offscreen(img);

  auto rgb_pipeline = std::make_shared<hut::rgb>(ofs);
  auto indices = buff->allocate<uint16_t>(6);
  indices->set({0, 1, 2, 2, 1, 3});
  auto vertices = buff->allocate<hut::rgb::vertex>(4);
  vertices->set({
    hut::rgb::vertex{{0, 0}, {1, 1, 1}},
    hut::rgb::vertex{{0, 2}, {1, 1, 1}},
    hut::rgb::vertex{{2, 0}, {1, 1, 1}},
    hut::rgb::vertex{{2, 2}, {1, 1, 1}},
  });
  auto instances = buff->allocate<hut::rgb::instance>(2);
  instances->set({
    hut::rgb::instance{hut::make_transform_mat4({0, 0}, {1,1,1})},
    hut::rgb::instance{hut::make_transform_mat4({2, 2}, {1,1,1})}
  });

  hut::proj_ubo default_ubo;
  default_ubo.proj_ = glm::ortho<float>(0, iparams.size_.x, 0, iparams.size_.y);
  auto ubo = d.alloc_ubo(buff, default_ubo);
  rgb_pipeline->write(0, ubo);

  d.flush_staged(); // staging has to be explicitly flushed in offscreen mode

  ofs.draw([&](VkCommandBuffer _cb) {
    rgb_pipeline->draw(_cb, 0, indices, instances, vertices);
  });

  glm::u8vec4 pixel_data[4 * 4];
  ofs.download(std::span<glm::u8>(&pixel_data[0].x, sizeof(pixel_data)), 4*4);

  constexpr glm::u8vec4 B = {0, 0, 0, 255};
  constexpr glm::u8vec4 W = {255, 255, 255, 255};
  glm::u8vec4 pixel_ref[4*4] = {
      W, W, B, B,
      W, W, B, B,
      B, B, W, W,
      B, B, W, W,
  };

 dump(pixel_data, {0, 0, 4, 4});
  EXPECT_TRUE(std::equal(std::begin(pixel_ref), std::end(pixel_ref), std::begin(pixel_data)));
}

TEST(offscreen, offscreen_download_offset) {
  hut::display d("offscreen_download_offset");

  auto buff = d.alloc_buffer(1024*1024);

  hut::image_params iparams;
  iparams.size_ = {4, 4};
  iparams.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  iparams.usage_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<hut::image>(d, iparams);

  {
    // Set pixel at [2,2]
    auto updator = img->prepare_update();
    auto *ptr = updator.data();
    memcpy(ptr + (2 * updator.staging_row_pitch() + 2 * 4), &W, 4);
  }

  auto ofs = hut::offscreen(img);

  d.flush_staged(); // staging has to be explicitly flushed in offscreen mode

  glm::u8vec4 pixel_data[5 * 5] = {
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
  };
  ofs.download(std::span<glm::u8>(&pixel_data[0].x, sizeof(pixel_data)), 5*4, hut::image::subresource{{2,2,4,4}});

  glm::u8vec4 pixel_ref[5 * 5] = {
      W, D, I, I, I,
      D, D, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
      I, I, I, I, I,
  };

  dump(pixel_data, {0, 0, 5, 5});
  EXPECT_TRUE(std::equal(std::begin(pixel_ref), std::end(pixel_ref), std::begin(pixel_data)));
}

TEST(offscreen, offscreen_render_offset) {
  hut::display d("offscreen_render_offset");

/*#ifdef HUT_ENABLE_RENDERDOC
  hut::renderdoc::frame_begin();
#endif*/

  auto buff = d.alloc_buffer(1024*1024);

  hut::image_params iparams;
  iparams.size_ = {4, 4};
  iparams.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  iparams.usage_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  auto img = std::make_shared<hut::image>(d, iparams);

  hut::offscreen_params oparams;
  oparams.subres_.coords_ = hut::make_bbox_with_origin_size(glm::u16vec2{1, 1}, {2, 2});
  auto ofs = hut::offscreen(img, oparams);

  auto rgb_pipeline = std::make_shared<hut::rgb>(ofs);
  auto indices = buff->allocate<uint16_t>(6);
  indices->set({0, 1, 2, 2, 1, 3});
  auto vertices = buff->allocate<hut::rgb::vertex>(4);
  vertices->set({
    hut::rgb::vertex{{0, 0}, {1, 1, 1}},
    hut::rgb::vertex{{0, 1}, {1, 1, 1}},
    hut::rgb::vertex{{1, 0}, {1, 1, 1}},
    hut::rgb::vertex{{1, 2}, {1, 1, 1}},
  });
  auto instances = buff->allocate<hut::rgb::instance>(1);
  instances->set({
    hut::rgb::instance{hut::make_transform_mat4({0, 0}, {1,1,1})},
  });

  hut::proj_ubo default_ubo;
  default_ubo.proj_ = glm::ortho<float>(0, 3, 0, 3);
  auto ubo = d.alloc_ubo(buff, default_ubo);
  rgb_pipeline->write(0, ubo);

  d.flush_staged(); // staging has to be explicitly flushed in offscreen mode

  ofs.draw([&](VkCommandBuffer _cb) {
    rgb_pipeline->draw(_cb, 0, indices, instances, vertices);
  });

  glm::u8vec4 pixel_data[4 * 4] = {
      I, I, I, I,
      I, I, I, I,
      I, I, I, I,
      I, I, I, I,
  };
  ofs.download(std::span<glm::u8>(&pixel_data[0].x, sizeof(pixel_data)), 4*4, hut::image::subresource{{0,0,4,4}});

  // Draw one white pixel at [2,2] (W), rest of the 3*3 offscreen should have been cleared (C), rest of the 4*4 image should be untouched,
  glm::u8vec4 pixel_ref[4 * 4] = {
      D, D, D, D,
      D, W, C, D,
      D, C, C, D,
      D, D, D, D,
  };

  dump(pixel_data, {0, 0, 4, 4});
  EXPECT_TRUE(std::equal(std::begin(pixel_ref), std::end(pixel_ref), std::begin(pixel_data)));

/*#ifdef HUT_ENABLE_RENDERDOC
  hut::renderdoc::frame_end("offscreen_render_offset");
#endif*/
}
