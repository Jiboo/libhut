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

#include "hut/display.hpp"
#include "hut/window.hpp"

#include "hut/ui/root.hpp"

#include "tst_events.hpp"
#include "tst_woff2.hpp"

using namespace hut;

int main(int /*unused*/, char ** /*unused*/) {
  display       dsp("hut ui playground");
  shared_buffer buf = std::make_shared<buffer>(dsp);

  window_params wp;
  wp.flags_.set(window_params::FTRANSPARENT);
  window win(dsp, buf, wp);
  win.title(u8"hut ui playground");
  win.clear_color({0, 0, 0, 1});

  auto fnt = std::make_shared<text::font>(tst_woff2::Roboto_Regular_woff2, 16_px);

  ui::root top(dsp, win, buf, fnt);

  dsp.dispatch();

  return EXIT_SUCCESS;
}
