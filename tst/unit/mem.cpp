#include <gtest/gtest.h>

#include "hut/buffer.hpp"

TEST(mem, simple) {
  hut::display d("testbed");

  hut::buffer b(d, 32,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  auto ref1 = b.allocate<float>(1);
  ASSERT_EQ(ref1->offset_, 0);
  ASSERT_EQ(ref1->size_, 4);

  auto ref2 = b.allocate<float>(1);
  ASSERT_EQ(ref2->offset_, 4);
  ASSERT_EQ(ref2->size_, 4);

  auto ref3 = b.allocate<float>(1);
  ASSERT_EQ(ref3->offset_, 8);
  ASSERT_EQ(ref3->size_, 4);

  ref2.reset();
  auto ref4 = b.allocate<float>(1);
  ASSERT_EQ(ref4->offset_, 4);
  ASSERT_EQ(ref4->size_, 4);

  ref4.reset();
  ref3.reset();
  auto ref5 = b.allocate<float>(2);
  ASSERT_EQ(ref5->offset_, 4);
  ASSERT_EQ(ref5->size_, 8);

  d.flush_staged();
}

TEST(mem, grow) {
  hut::display d("testbed");

  hut::buffer b(d, 16,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));

  auto ref1 = b.allocate<float>(32);
  ref1->set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
             16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31});
  ASSERT_EQ(ref1->offset_, 0);
  ASSERT_EQ(ref1->size_, 32 * 4);

  d.flush_staged();
}
