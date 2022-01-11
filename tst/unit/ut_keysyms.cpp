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

#if __has_include(<linux/input.h>)
  #include <linux/input.h>

  #define HUT_MAP_KEYSYM(FORMAT_LINUX, FORMAT_X11, FORMAT_IMGUI) int test_linux_##FORMAT_LINUX = KEY_##FORMAT_LINUX;
  #include "hut/keysyms.inc"
  #undef HUT_MAP_KEYSYM
#endif

#if __has_include(<X11/keysym.h>)
  #include <X11/keysym.h>

  #define HUT_MAP_KEYSYM(FORMAT_LINUX, FORMAT_X11, FORMAT_IMGUI) int test_x11_##FORMAT_X11 = XK_##FORMAT_X11;
  #include "hut/keysyms.inc"
  #undef HUT_MAP_KEYSYM
#endif
