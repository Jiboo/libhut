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

    void display::keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
            uint32_t format, int fd, uint32_t size) {
    }

    void display::keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
            uint32_t serial, struct wl_surface *surface,
            struct wl_array *keys) {
    }

    void display::keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
            uint32_t serial, struct wl_surface *surface) {
    }

    void display::keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
            uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
        //struct display *d = (display*)data;

        /*TODO if (key == KEY_F11 && state) {
            if (d->window->fullscreen)
                xdg_surface_unset_fullscreen(d->window->xdg_surface);
            else
                xdg_surface_set_fullscreen(d->window->xdg_surface, NULL);
        } else if (key == KEY_ESC && state)
            running = 0;*/
    }

    void display::keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
            uint32_t serial, uint32_t mods_depressed,
            uint32_t mods_latched, uint32_t mods_locked,
            uint32_t group) {
    }

    void display::keyboard_handle_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
            int32_t rate, int32_t delay) {
    }

} //namespace hut