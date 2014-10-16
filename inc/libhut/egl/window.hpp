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

#include "libhut/window.hpp"

#include "libhut/egl/surface.hpp"

namespace hut {

    class egl_window : public base_window {
    public:
        egl_window(display& dpy, const std::string& title, bool translucent = false, window_decoration_type deco = DSYSTEM)
                : base_window(dpy, title, translucent, deco) {
        }

        ~egl_window() {
            eglMakeCurrent(this->dpy.egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroySurface(this->dpy.egl_dpy, this->egl_surf);
        }

    protected:
        EGLSurface egl_surf;

        void init_egl_window(EGLNativeWindowType win) {
            egl_surf = eglCreateWindowSurface(this->dpy.egl_dpy, this->dpy.egl_conf, win, NULL);
        }
    };

} //namespace hut