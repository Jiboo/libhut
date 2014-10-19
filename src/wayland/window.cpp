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

    static const char *vert_shader_text =
            "uniform mat4 rotation;\n"
            "attribute vec4 pos;\n"
            "attribute vec4 color;\n"
            "varying vec4 v_color;\n"
            "void main() {\n"
            "  gl_Position = rotation * pos;\n"
            "  v_color = color;\n"
            "}\n";

    static const char *frag_shader_text =
            "precision mediump float;\n"
            "varying vec4 v_color;\n"
            "void main() {\n"
            "  gl_FragColor = v_color;\n"
            "}\n";

    static GLuint
    create_shader(window *window, const char *source, GLenum shader_type)
    {
        GLuint shader;
        GLint status;

        shader = glCreateShader(shader_type);
        assert(glGetError() == GL_NO_ERROR);
        assert(shader != 0);

        glShaderSource(shader, 1, (const char **) &source, NULL);
        glCompileShader(shader);

        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status) {
            char log[1000];
            GLsizei len;
            glGetShaderInfoLog(shader, 1000, &len, log);
            fprintf(stderr, "Error: compiling %s: %*s\n",
                    shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
                    len, log);
            exit(1);
        }

        return shader;
    }

    static GLint rotation_uniform;

    static void
    init_gl(window *window)
    {
        GLuint frag, vert;
        GLuint program;
        GLint status;

        printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
        printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
        printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
        printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));

        glClearColor(0.4, 0.4, 0.4, 0.0);
        assert(glGetError() == GL_NO_ERROR);

        frag = create_shader(window, frag_shader_text, GL_FRAGMENT_SHADER);
        vert = create_shader(window, vert_shader_text, GL_VERTEX_SHADER);

        program = glCreateProgram();
        glAttachShader(program, frag);
        glAttachShader(program, vert);
        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (!status) {
            char log[1000];
            GLsizei len;
            glGetProgramInfoLog(program, 1000, &len, log);
            fprintf(stderr, "Error: linking:\n%*s\n", len, log);
            exit(1);
        }

        glUseProgram(program);

        glBindAttribLocation(program, 0, "pos");
        glBindAttribLocation(program, 1, "color");
        glLinkProgram(program);

        rotation_uniform = glGetUniformLocation(program, "rotation");
    }

    static time_t benchmark_time;
    static int frames;

    void
    redraw_gl(window *window)
    {
        display &display = window->dpy;
        static const GLfloat verts[3][2] = {
                { -0.5, -0.5 },
                {  0.5, -0.5 },
                {  0,    0.5 }
        };
        static const GLfloat colors[3][3] = {
                { 1, 0, 0 },
                { 0, 1, 0 },
                { 0, 0, 1 }
        };
        GLfloat angle;
        GLfloat rotation[4][4] = {
                { 1, 0, 0, 0 },
                { 0, 1, 0, 0 },
                { 0, 0, 1, 0 },
                { 0, 0, 0, 1 }
        };
        static const uint32_t speed_div = 5, benchmark_interval = 5;
        struct timeval tv;

        gettimeofday(&tv, NULL);
        time_t time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        if (frames == 0)
            benchmark_time = time;
        if (time - benchmark_time > (benchmark_interval * 1000)) {
            printf("%d frames in %d seconds: %f fps\n",
                    frames,
                    benchmark_interval,
                    (float) frames / benchmark_interval);
            benchmark_time = time;
            frames = 0;
        }

        angle = (time / speed_div) % 360 * M_PI / 180.0;
        rotation[0][0] =  cos(angle);
        rotation[0][2] =  sin(angle);
        rotation[2][0] = -sin(angle);
        rotation[2][2] =  cos(angle);

        glUniformMatrix4fv(rotation_uniform, 1, GL_FALSE, (GLfloat *) rotation);

        glClearColor(0.0, 0.0, 0.0, 0.5);
        glClear(GL_COLOR_BUFFER_BIT);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, colors);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }

    window::window(display& dpy, const std::string& title, bool translucent, window_decoration_type deco)
            : egl_window(dpy, title, translucent, deco) {

        wl_surf = wl_compositor_create_surface(dpy.compositor);
        xdg_surface = xdg_shell_get_xdg_surface(dpy.shell, wl_surf);

        static const struct xdg_surface_listener xdg_surface_listeners = {
                handle_surface_configure,
                handle_surface_delete,
        };

        xdg_surface_add_listener(xdg_surface, &xdg_surface_listeners, this);

        wl_win = wl_egl_window_create(wl_surf, buffed_size.get()[0], buffed_size.get()[1]);
        init_egl_window(wl_win);

        xdg_surface_set_title(xdg_surface, title.c_str());

        //FIXME
        //xdg_surface_set_fullscreen(xdg_surface, NULL);

        assert(eglMakeCurrent(dpy.egl_dpy, egl_surf, egl_surf, dpy.egl_ctx) == EGL_TRUE);

        GLint ret = eglSwapInterval(dpy.egl_dpy, 0);
        runtime_assert(ret == EGL_TRUE, "eglSwapInterval failed");

        init_gl(this);

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

    void window::draw(const hut::mesh&) {

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
        thiz->buffed_size = {{width, height}};

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

            redraw_gl(this);

            if (fullscreen) {
                struct wl_region *region = wl_compositor_create_region(dpy.compositor);
                wl_region_add(region, 0, 0,
                        buffed_size.get()[0],
                        buffed_size.get()[1]);
                wl_surface_set_opaque_region(wl_surf, region);
                wl_region_destroy(region);
            } else {
                wl_surface_set_opaque_region(wl_surf, NULL);
            }

            ret = eglSwapBuffers(dpy.egl_dpy, egl_surf);
            assert(ret == EGL_TRUE);

            frames++;
            //redraw = false; FIXME
        }
    }

} //namespace hut
