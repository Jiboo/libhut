#include <gtest/gtest.h>

#include "hut/display.hpp"
#include "hut/buffer_pool.hpp"

TEST(mem, simple) {
  hut::display d("testbed");

  hut::buffer_pool b(d, 32,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  auto ref1 = b.allocate<float>(1);
  EXPECT_EQ(ref1->offset_bytes(), 0);
  EXPECT_EQ(ref1->size_bytes(), 4);

  auto ref2 = b.allocate<float>(1);
  EXPECT_EQ(ref2->offset_bytes(), 4);
  EXPECT_EQ(ref2->size_bytes(), 4);

  auto ref3 = b.allocate<float>(1);
  EXPECT_EQ(ref3->offset_bytes(), 8);
  EXPECT_EQ(ref3->size_bytes(), 4);

  ref2.reset();

  auto ref4 = b.allocate<float>(1);
  EXPECT_EQ(ref4->offset_bytes(), 4);
  EXPECT_EQ(ref4->size_bytes(), 4);

  ref4.reset();
  ref3.reset();

  auto ref5 = b.allocate<float>(2);
  EXPECT_EQ(ref5->offset_bytes(), 4);
  EXPECT_EQ(ref5->size_bytes(), 8);

  d.flush_staged();
}

TEST(mem, grow) {
  hut::display d("testbed");

  hut::buffer_pool b(d, 16,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));

  auto ref0 = b.allocate<float>(2);
  EXPECT_EQ(ref0->offset_bytes(), 0);

  auto ref1 = b.allocate<float>(32);
  d.flush_staged();

  ref1->set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
             16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31});
  EXPECT_EQ(ref1->offset_bytes(), 0);
  EXPECT_EQ(ref1->size_bytes(), 32 * 4);
  EXPECT_NE(ref0->vkBuffer(), ref1->vkBuffer());

  auto ref2 = b.allocate<float>(32);
  EXPECT_EQ(ref2->size_bytes(), 32 * 4);
  EXPECT_NE(ref1->vkBuffer(), ref2->vkBuffer());

  d.flush_staged();
}

TEST(mem, align) {
  hut::display d("testbed");

  hut::buffer_pool b(d, 1024,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));

  auto ref1 = b.allocate<float>(10, 0x80);
  EXPECT_EQ(ref1->offset_bytes(), 0);
  EXPECT_EQ(ref1->size_bytes(), 10 * 4);

  auto ref2 = b.allocate<float>(10, 0x80);
  EXPECT_EQ(ref2->offset_bytes(), 0x80);
  EXPECT_EQ(ref2->size_bytes(), 10 * 4);

  d.flush_staged();
}
