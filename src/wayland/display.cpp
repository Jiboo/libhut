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
#include "libhut/wayland/xdg-shell-client-protocol.h"

#include "libhut/wayland/display.hpp"
#include "libhut/wayland/window.hpp"

namespace hut {

    void display::seat_handle_capabilities(void *data, struct wl_seat *seat,
            uint32_t caps) {
        static const struct wl_pointer_listener pointer_listeners = {
                pointer_handle_enter,
                pointer_handle_leave,
                pointer_handle_motion,
                pointer_handle_button,
                pointer_handle_axis,
        };

        struct display *d = (display*)data;

        if ((caps & WL_SEAT_CAPABILITY_POINTER) && !d->pointer) {
            d->pointer = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(d->pointer, &pointer_listeners, d);
        } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && d->pointer) {
            wl_pointer_destroy(d->pointer);
            d->pointer = NULL;
        }

        static const struct wl_keyboard_listener keyboard_listeners = {
                keyboard_handle_keymap,
                keyboard_handle_enter,
                keyboard_handle_leave,
                keyboard_handle_key,
                keyboard_handle_modifiers,
                keyboard_handle_repeat_info
        };

        if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !d->keyboard) {
            d->keyboard = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(d->keyboard, &keyboard_listeners, d);
        } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && d->keyboard) {
            wl_keyboard_destroy(d->keyboard);
            d->keyboard = NULL;
        }

        static const struct wl_touch_listener touch_listeners = {
                touch_handle_down,
                touch_handle_up,
                touch_handle_motion,
                touch_handle_frame,
                touch_handle_cancel,
        };

        if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !d->touch) {
            d->touch = wl_seat_get_touch(seat);
            wl_touch_set_user_data(d->touch, d);
            wl_touch_add_listener(d->touch, &touch_listeners, d);
        } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && d->touch) {
            wl_touch_destroy(d->touch);
            d->touch = NULL;
        }
    }

    void display::seat_handle_name(void *data, struct wl_seat *wl_seat,
            const char *name) {
    }

    void display::shell_ping(void *data, struct xdg_shell *shell,
            uint32_t serial) {
        xdg_shell_pong(shell, serial);
    }

    void display::registry_handle_global(void *data, struct wl_registry *registry,
            uint32_t name, const char *interface, uint32_t version) {
        display *d = (display*)data;

        static const struct wl_seat_listener seat_listeners = {
                seat_handle_capabilities,
                seat_handle_name
        };

        static const struct xdg_shell_listener shell_listeners = {
                &hut::display::shell_ping,
        };

        if (strcmp(interface, "wl_compositor") == 0) {
            d->compositor = (wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, 1);

        } else if (strcmp(interface, "xdg_shell") == 0) {
            d->shell = (xdg_shell*) wl_registry_bind(registry, name, &xdg_shell_interface, 1);
            xdg_shell_add_listener(d->shell, &shell_listeners, d);
            xdg_shell_use_unstable_version(d->shell, XDG_VERSION);

        } else if (strcmp(interface, "wl_seat") == 0) {
            d->seat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);
            wl_seat_add_listener(d->seat, &seat_listeners, d);

        } else if (strcmp(interface, "wl_shm") == 0) {
            d->shm = (wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, 1);
            d->cursor_theme = wl_cursor_theme_load(NULL, 32, d->shm);
            if (!d->cursor_theme) {
                fprintf(stderr, "unable to load default theme\n");
                return;
            }
            d->default_cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "left_ptr");
            if (!d->default_cursor) {
                fprintf(stderr, "unable to load default left pointer\n");
                // TODO: abort ?
            }
        }
    }

    void display::registry_handle_global_remove(void *data, struct wl_registry *registry,
            uint32_t name) {
    }

    display::display(const char* name) {
        wl_dpy = wl_display_connect(name);
        runtime_assert(wl_dpy != nullptr, "wl_display_connect failed");

        registry = wl_display_get_registry(wl_dpy);
        wl_registry_add_listener(registry, &registry_listener(), this);
        wl_display_dispatch(wl_dpy);

        egl_init_display(wl_dpy);

        printf("EGL_VERSION = %s\n", eglQueryString(egl_dpy, EGL_VERSION));
        printf("EGL_VENDOR = %s\n", eglQueryString(egl_dpy, EGL_VENDOR));
        printf("EGL_EXTENSIONS = %s\n", eglQueryString(egl_dpy, EGL_EXTENSIONS));

        xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        runtime_assert(xkb_context != nullptr, "xkb_context_new failed");
    }

    display::~display() {
        wl_surface_destroy(cursor_surface);
        if (cursor_theme)
            wl_cursor_theme_destroy(cursor_theme);

        if (shell)
            xdg_shell_destroy(shell);

        if (compositor)
            wl_compositor_destroy(compositor);

        wl_registry_destroy(registry);

        wl_display_flush(wl_dpy);
        wl_display_disconnect(wl_dpy);

        xkb_state_unref(xkb_state);
        xkb_keymap_unref(xkb_keymap);
    }

    void display::flush() {
        wl_display_flush(wl_dpy);
    }

    void display::dispatch_draw() {
        //TODO Call pending posted jobs
        for(window* win : active_wins) {
            win->dispatch_draw();
        }
    }

} //namespace hut
