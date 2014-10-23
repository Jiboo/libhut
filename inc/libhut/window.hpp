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

#include <string>

#include "libhut/vec.hpp"
#include "libhut/event.hpp"
#include "libhut/property.hpp"

#include "libhut/display.hpp"
#include "libhut/surface.hpp"
#include "libhut/window.hpp"

namespace hut {

    enum touch_event_type {
        TDOWN, TUP, TMOVE
    };

    enum mouse_event_type {
        MDOWN, MUP, MMOVE, MENTER, MLEAVE, MWHEEL_UP, MWHEEL_DOWN
    };

    enum keyboard_control_type {
        KALT_LEFT, KALT_RIGHT,
        KCTRL_LEFT, KCTRL_RIGHT,
        KSHIFT_LEFT, KSHIFT_RIGHT,
        KPAGE_UP, KPAGE_DOWN,
        KVOLUME_UP, KVOLUME_DOWN,
        KUP, KRIGHT, KDOWN, KLEFT,
        KBACKSPACE, KDELETE, KINSER,
        KESCAPE,
        KF1, KF2, KF3, KF4, KF5, KF6, KF7, KF8, KF9,
    };

    enum window_decoration_type {
        DSYSTEM,
        DFULLSCREEN,
        DOVERRIDE
    };

    class view_node {
    public:
        view_node* parent;

        view_node(view_node* parent) : parent(parent) {}
    };

    class base_window : public base_surface, public view_node {
    public:

        event<> on_pause, on_resume;
        event<std::string /*path*/, vec2 /*pos*/> on_drop;

        event<uint8_t /*finger*/, touch_event_type, ivec2 /*pos*/> on_touch;
        event<uint8_t /*button*/, mouse_event_type, ivec2 /*pos*/> on_mouse;
        event<uint8_t /*axis*/, float /*pos*/> on_axis;
        event<char32_t /*utf32_char*/, bool /*down*/> on_char;
        event<keyboard_control_type, bool /*down*/> on_ctrl;

        buffed<bool> fullscreen { [this](auto b) { this->enable_fullscreen(b); }, false };
        buffed<bool> maximized { [this](auto b) { this->enable_maximize(b); }, false };

        buffed<ivec2> geometry{ [this](auto v) { this->resize(v); }, {{500, 500}} };
        buffed<std::string> title { [this](auto s) { this->rename(s); }, "" };

        display& dpy;

        base_window(display& dpy, const std::string& title, bool translucent = false, window_decoration_type deco = DSYSTEM)
                : view_node(nullptr), dpy(dpy) {
        }

        virtual ~base_window() {
            cleanup();
        }

        virtual void close() = 0;

        virtual void minimize() = 0;

    protected:
        /* Used as a second destructor pass */
        virtual void cleanup() {

        }

        virtual void enable_fullscreen(bool enable) = 0;

        virtual void enable_maximize(bool enable) = 0;

        virtual void resize(ivec2 size) = 0;

        virtual void rename(const std::string& name) = 0;
    };

} // namespace hut

#ifdef HUT_WAYLAND

#include "libhut/wayland/window.hpp"

#endif