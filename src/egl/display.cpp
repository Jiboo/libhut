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

#include <type_traits>

#include <wayland-client.h>
#include <wayland-egl.h>

#include "libhut/egl/display.hpp"
#include "libhut/utils.hpp"
#include "libhut/vec.hpp"

namespace hut {

    void egl_display::egl_init(NativeDisplayType display) {
        static const EGLint context_attribs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL_NONE
        };

        static EGLint config_attribs[] = {
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_RED_SIZE, 1,
                EGL_GREEN_SIZE, 1,
                EGL_BLUE_SIZE, 1,
                EGL_ALPHA_SIZE, 1,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_NONE
        };

        egl_dpy = eglGetDisplay(display);

        EGLint major, minor;
        EGLBoolean ret = eglInitialize(egl_dpy, &major, &minor);
        runtime_assert(ret == EGL_TRUE, "eglInitialize failed");

        ret = eglBindAPI(EGL_OPENGL_ES_API);
        runtime_assert(ret == EGL_TRUE, "eglBindAPI failed");

        EGLint count;
        ret = eglGetConfigs(egl_dpy, NULL, 0, &count);
        runtime_assert(ret == EGL_TRUE, "eglGetConfigs failed");
        runtime_assert(count != 0, "eglGetConfigs failed with count = 0");

        EGLConfig *configs = (EGLConfig*)calloc(count, sizeof *configs);
        runtime_assert(configs != nullptr, "egl_display::init calloc failed");

        EGLint n;
        ret = eglChooseConfig(egl_dpy, config_attribs, configs, count, &n);
        runtime_assert(ret && n >= 1, "eglChooseConfig failed");

        for(size_t i = 0; i < n; i++) {
            EGLint size;
            eglGetConfigAttrib(egl_dpy, configs[i], EGL_BUFFER_SIZE, &size);
            if (size == 32) {
                egl_conf = configs[i];
                break;
            }
        }
        free(configs);
        runtime_assert(ret && n >= 1, "did not find config with buffer size 32");

        egl_ctx = eglCreateContext(egl_dpy, egl_conf, EGL_NO_CONTEXT, context_attribs);
        runtime_assert(egl_ctx != nullptr, "eglCreateContext failed");
    }

} //namespace hut
