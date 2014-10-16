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

#include "xdg-shell-client-protocol.h"

#include "libhut/egl/window.hpp"
#include "libhut/wayland/surface.hpp"
#include "libhut/wayland/display.hpp"

namespace hut {

    class window : public egl_window {
        friend class display;
        friend void redraw_gl(window *window);

    public:
        window(display& dpy, const std::string& title, bool translucent = false,
                window_decoration_type deco = DSYSTEM);

        ~window() {
            dpy.active_wins.remove(this);
            wl_egl_window_destroy(wl_win);
        }

        virtual ivec2 size() const;
        virtual short unsigned int density() const;
        virtual void draw(const hut::mesh&);
        virtual void draw(const hut::batch&);

        virtual void minimize();
        virtual void maximize();
        virtual void close();
        virtual void enable_fullscreen(bool);
        virtual void resize(hut::ivec2);
        virtual void rename(std::string);

    protected:
        struct wl_egl_window *wl_win;
        struct wl_surface *wl_surf;
        struct xdg_surface *xdg_surface;
        bool redraw = false;

        virtual void cleanup() {
            egl_window::cleanup();
            xdg_surface_destroy(xdg_surface);
            wl_surface_destroy(wl_surf);
        }

        static void handle_surface_delete(void *data, struct xdg_surface *xdg_surface);
        static void handle_surface_configure(void *data, struct xdg_surface *surface,
                int32_t width, int32_t height,
                struct wl_array *states, uint32_t serial);

        virtual void dispatch_draw();
    };

} //namespace hut
