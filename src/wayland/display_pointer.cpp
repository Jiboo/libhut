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

    void display::pointer_handle_enter(void *data, struct wl_pointer *pointer,
            uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy) {
        display *d = (display*)data;
        struct wl_buffer *buffer;
        struct wl_cursor *cursor = d->default_cursor;
        struct wl_cursor_image *image;

        if (false) // TODO window->fullscreen
            wl_pointer_set_cursor(pointer, serial, NULL, 0, 0);
        else if (cursor) {
            image = d->default_cursor->images[0];
            buffer = wl_cursor_image_get_buffer(image);
            if (!buffer)
                return;
            wl_pointer_set_cursor(pointer, serial,
                    d->cursor_surface,
                    image->hotspot_x,
                    image->hotspot_y);
            wl_surface_attach(d->cursor_surface, buffer, 0, 0);
            wl_surface_damage(d->cursor_surface, 0, 0,
                    image->width, image->height);
            wl_surface_commit(d->cursor_surface);
        }
    }

    void display::pointer_handle_leave(void *data, struct wl_pointer *pointer,
            uint32_t serial, struct wl_surface *surface) {
    }

    void display::pointer_handle_motion(void *data, struct wl_pointer *pointer,
            uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
    }

    void display::pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
            uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
        //struct display *display = (display*)data;

        /*TODO if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
            xdg_surface_move(display->window->xdg_surface, display->seat, serial);*/
    }

    void display::pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
            uint32_t time, uint32_t axis, wl_fixed_t value) {
    }

} //namespace hut