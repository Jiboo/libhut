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

#include <cassert>

#include <iostream>

#include <GLES2/gl2.h>
#include <sys/time.h>

#include "libhut/wayland/window.hpp"

namespace hut {

    window::window(display& dpy, const std::string& title, bool translucent, window_decoration_type deco)
            : egl_window(dpy, title, translucent, deco) {

        wl_surf = wl_compositor_create_surface(dpy.compositor);
        xdg_surface = xdg_shell_get_xdg_surface(dpy.shell, wl_surf);

        static const struct xdg_surface_listener xdg_surface_listeners = {
                handle_surface_configure,
                handle_surface_delete,
        };

        xdg_surface_add_listener(xdg_surface, &xdg_surface_listeners, this);

        wl_win = wl_egl_window_create(wl_surf, geometry.get()[0], geometry.get()[1]);
        init_egl_window(wl_win);

        xdg_surface_set_title(xdg_surface, title.c_str());

        //FIXME
        //xdg_surface_set_fullscreen(xdg_surface, NULL);

        assert(eglMakeCurrent(dpy.egl_dpy, egl_surf, egl_surf, dpy.egl_ctx) == EGL_TRUE);

        GLint ret = eglSwapInterval(dpy.egl_dpy, 0);
        runtime_assert(ret == EGL_TRUE, "eglSwapInterval failed");

        dpy.active_wins.emplace_back(this);
    }

    ivec2 window::size() const {
        ivec2 result;
        wl_egl_window_get_attached_size(wl_win, result.data, result.data + 1);
        return result;
    }

    short unsigned int window::density() const {
        return 160; //FIXME
    }

    void window::draw(const hut::drawable& d) {
        d.draw();
    }

    void window::draw(const hut::batch&) {

    }

    void window::minimize() {
        xdg_surface_set_minimized(xdg_surface);
    }

    void window::close() {
        if(xdg_surface)
            xdg_surface_destroy(xdg_surface);

        if(wl_surf)
            wl_surface_destroy(wl_surf);
    }

    void window::enable_fullscreen(bool enable) {
        if(enable)
            xdg_surface_set_fullscreen(xdg_surface, NULL);
        else
            xdg_surface_unset_fullscreen(xdg_surface);
    }

    void window::enable_maximize(bool enable) {
        if(enable)
            xdg_surface_set_maximized(xdg_surface);
        else
            xdg_surface_unset_maximized(xdg_surface);
    }

    void window::resize(hut::ivec2 size) {
        if (wl_win)
            wl_egl_window_resize(wl_win, size[0], size[1], 0, 0);
    }

    void window::rename(const std::string& title) {
        xdg_surface_set_title(xdg_surface, title.c_str());
    }

    void window::handle_surface_delete(void *data, struct xdg_surface *xdg_surface) {
        //window* thiz = (window*)data;
    }

    void window::handle_surface_configure(void *data, struct xdg_surface *surface,
            int32_t width, int32_t height,
            struct wl_array *states, uint32_t serial) {
        window* thiz = (window*)data;

        thiz->fullscreen.cache(false);
        thiz->maximized.cache(false);
        thiz->geometry = {{width, height}};

        #ifndef __CLDOC__
        void *p;
        wl_array_for_each(p, states) {
            uint32_t state = *((uint32_t*)p);
            switch(state) {
                case XDG_SURFACE_STATE_FULLSCREEN:
                    thiz->fullscreen.cache(true);
                    break;
                case XDG_SURFACE_STATE_MAXIMIZED:
                    thiz->maximized.cache(true);
            }
        }
        #endif

        if(thiz->wl_win)
            wl_egl_window_resize(thiz->wl_win, width, height, 0, 0);

        thiz->redraw = true;

        xdg_surface_ack_configure(surface, serial);
    }

    void window::dispatch_draw() {
        if(redraw) {
            EGLBoolean ret = eglMakeCurrent(dpy.egl_dpy, egl_surf,
                    egl_surf, dpy.egl_ctx);
            assert(ret == EGL_TRUE);

            glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
            glClear(GL_COLOR_BUFFER_BIT);

            glViewport(0, 0, geometry.get()[0], geometry.get()[1]);

            on_draw.fire();

            if (fullscreen) {
                struct wl_region *region = wl_compositor_create_region(dpy.compositor);
                wl_region_add(region, 0, 0,
                        geometry.get()[0],
                        geometry.get()[1]);
                wl_surface_set_opaque_region(wl_surf, region);
                wl_region_destroy(region);
            } else {
                wl_surface_set_opaque_region(wl_surf, NULL);
            }

            ret = eglSwapBuffers(dpy.egl_dpy, egl_surf);
            assert(ret == EGL_TRUE);

            assert(glGetError() == GL_NO_ERROR);
            assert(eglGetError() == EGL_SUCCESS);

            //redraw = false; FIXME
        }
    }

} //namespace hut
