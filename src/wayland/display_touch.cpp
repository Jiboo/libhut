/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste "Jiboo" Lepesme
 * github.com/jiboo/libhut @lepesmejb +JeanBaptisteLepesme
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <wayland-cursor.h>

#include "libhut/wayland/display.hpp"

namespace hut {

    void display::touch_handle_down(void *data, struct wl_touch *wl_touch,
            uint32_t serial, uint32_t time, struct wl_surface *surface,
            int32_t id, wl_fixed_t x_w, wl_fixed_t y_w) {
        //struct display *d = (struct display *)data;

        //TODO xdg_surface_move(d->window->xdg_surface, d->seat, serial);
    }

    void display::touch_handle_up(void *data, struct wl_touch *wl_touch,
            uint32_t serial, uint32_t time, int32_t id) {
    }

    void display::touch_handle_motion(void *data, struct wl_touch *wl_touch,
            uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w) {
    }

    void display::touch_handle_frame(void *data, struct wl_touch *wl_touch) {
    }

    void display::touch_handle_cancel(void *data, struct wl_touch *wl_touch) {
    }

} //namespace hut