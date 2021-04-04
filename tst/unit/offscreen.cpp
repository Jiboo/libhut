#include <gtest/gtest.h>

#include "hut/offscreen.hpp"
#include "hut/pipeline.hpp"

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
  auto img = std::make_shared<hut::image>(d, params);

  auto ofs = std::make_shared<hut::offscreen>(img);
}

TEST(offscreen, pipeline_noop) {
  hut::display d("pipeline_noop");

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
  ofs.download((uint8_t*)pixel_data, 4*4);

  constexpr glm::u8vec4 B = {0, 0, 0, 255};
  constexpr glm::u8vec4 W = {255, 255, 255, 255};
  glm::u8vec4 pixel_ref[4*4] = {
      W, W, B, B,
      W, W, B, B,
      B, B, W, W,
      B, B, W, W,
  };

  EXPECT_TRUE(std::equal(std::begin(pixel_ref), std::end(pixel_ref), std::begin(pixel_data)));
}
