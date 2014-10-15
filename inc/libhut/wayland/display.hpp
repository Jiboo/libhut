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

#pragma once

#include <cstring>

#include <string>

#include <wayland-client.h>

#include "libhut/utils.hpp"
#include "libhut/egl/display.hpp"

#define XDG_VERSION 4 /* The version of xdg-shell that we implement */
#ifdef static_assert
    static_assert(XDG_VERSION == XDG_SHELL_VERSION_CURRENT, "Interface version doesn't match implementation version");
#endif

namespace hut {
    class display : public egl_display {
    public:
        display(const std::string &name);

        display(const char *name);

        virtual ~display();

        virtual void post(std::function<void()> calllback) override;

        virtual void post_delayed(std::function<void()> calllback, std::chrono::milliseconds delay);

        virtual void schedule(std::function<void()> calllback, std::chrono::milliseconds delay);

        virtual void flush();

    protected:
        struct wl_display *wl_dpy;
        struct wl_registry *registry;
        struct wl_compositor *compositor;
        struct xdg_shell *shell;
        struct wl_seat *seat;
        struct wl_pointer *pointer;
        struct wl_touch *touch;
        struct wl_keyboard *keyboard;
        struct wl_shm *shm;
        struct wl_cursor_theme *cursor_theme;
        struct wl_cursor *default_cursor;
        struct wl_surface *cursor_surface;

        static void pointer_handle_enter(void *data, struct wl_pointer *pointer,
                uint32_t serial, struct wl_surface *surface,
                wl_fixed_t sx, wl_fixed_t sy);

        static void pointer_handle_leave(void *data, struct wl_pointer *pointer,
                uint32_t serial, struct wl_surface *surface);

        static void pointer_handle_motion(void *data, struct wl_pointer *pointer,
                uint32_t time, wl_fixed_t sx, wl_fixed_t sy);

        static void pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                uint32_t serial, uint32_t time, uint32_t button,
                uint32_t state);

        static void pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
                uint32_t time, uint32_t axis, wl_fixed_t value);

        static void touch_handle_down(void *data, struct wl_touch *wl_touch,
                uint32_t serial, uint32_t time, struct wl_surface *surface,
                int32_t id, wl_fixed_t x_w, wl_fixed_t y_w);

        static void touch_handle_up(void *data, struct wl_touch *wl_touch,
                uint32_t serial, uint32_t time, int32_t id);

        static void touch_handle_motion(void *data, struct wl_touch *wl_touch,
                uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w);

        static void touch_handle_frame(void *data, struct wl_touch *wl_touch);

        static void touch_handle_cancel(void *data, struct wl_touch *wl_touch);

        static void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                uint32_t format, int fd, uint32_t size);

        static void keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                uint32_t serial, struct wl_surface *surface, struct wl_array *keys);

        static void keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                uint32_t serial, struct wl_surface *surface);

        static void keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                uint32_t serial, uint32_t time, uint32_t key, uint32_t state);

        static void keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
                uint32_t mods_locked, uint32_t group);

        static void keyboard_handle_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
                int32_t rate, int32_t delay);

        static void seat_handle_capabilities(void *data, struct wl_seat *seat,
                uint32_t caps);

        static void seat_handle_name(void *data, struct wl_seat *wl_seat,
                const char *name);

        static void shell_ping(void *data, struct xdg_shell *shell, uint32_t serial);

        static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);

        static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name);

        const wl_registry_listener &registry_listener() {
            static wl_registry_listener listeners = {

                    &hut::display::registry_handle_global,
                    &hut::display::registry_handle_global_remove
            };
            return listeners;
        }
    };

} //namespace hut
