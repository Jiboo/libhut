/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste Lepesme github.com/jiboo/libhut
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <gtest/gtest.h>

#include "hut/utils/flagged.hpp"

using namespace hut;

TEST(flagged, ctor) {
  enum option { F1, F2, F3, LAST_OPTION = F3 };
  using options = flagged<option, LAST_OPTION>;

  options a;
  EXPECT_TRUE(a.empty());

  {
    options b(a);
    EXPECT_TRUE(b.empty());
  }

  {
    options d(F1);
    EXPECT_FALSE(d.empty());
    EXPECT_TRUE(d.query(F1));
    EXPECT_FALSE(d.query(F2));
    EXPECT_FALSE(d.query(F3));
  }

  {
    options e{F1, F2};
    EXPECT_FALSE(e.empty());
    EXPECT_TRUE(e.query(F1));
    EXPECT_TRUE(e.query(F2));
    EXPECT_FALSE(e.query(F3));
  }
}
