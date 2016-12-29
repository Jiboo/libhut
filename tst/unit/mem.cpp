#include <gtest/gtest.h>

#include "hut/buffer.hpp"

TEST(mem, simple) {
  hut::display d("testbed");

  hut::buffer b(d, 32, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

  auto ref1 = b.allocate<float, 1>();
  ASSERT_EQ(ref1->offset_, 0);
  ASSERT_EQ(ref1->size_, 4);

  auto ref2 = b.allocate<float, 1>();
  ASSERT_EQ(ref2->offset_, 4);
  ASSERT_EQ(ref2->size_, 4);

  auto ref3 = b.allocate<float, 1>();
  ASSERT_EQ(ref3->offset_, 8);
  ASSERT_EQ(ref3->size_, 4);

  ref2.reset();
  auto ref4 = b.allocate<float, 1>();
  ASSERT_EQ(ref4->offset_, 4);
  ASSERT_EQ(ref4->size_, 4);

  ref4.reset();
  ref3.reset();
  auto ref5 = b.allocate<float, 2>();
  ASSERT_EQ(ref5->offset_, 4);
  ASSERT_EQ(ref5->size_, 8);
}

TEST(mem, grow) {
  hut::display d("testbed");

  hut::buffer b(d, 16, (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT));

  auto ref1 = b.allocate<float, 32>();
  ASSERT_EQ(ref1->offset_, 0);
  ASSERT_EQ(ref1->size_, 32 * 4);
}
