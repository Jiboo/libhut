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

#include "vec.hpp"
#include "event.hpp"

#include "libhut/surface.hpp"
#include "libhut/window.hpp"

namespace hut {

    enum class pointer_event_type {
        MDOWN, MUP, MMOVE
    };

    enum class control_type {
        KALT_LEFT, KALT_RIGHT,
        KCTRL_LEFT, KCTRL_RIGHT,
        KWHEEL_UP, KWHEEL_DOWN,
        KPAGE_UP, KPAGE_DOWN,
        KVOLUME_UP, KVOLUME_DOWN,
        KUP, KRIGHT, KDOWN, KLEFT,
        KBACKSPACE, KDELETE, KINSER,
        KESCAPE,
        KF1, KF2, KF3, KF4, KF5, KF6, KF7, KF8, KF9,
    };

    template<typename Tbase_surface = base_surface>
    class base_window : public Tbase_surface {
    public:
        base_window(bool transparent = false);

        event<> on_create, on_pause, on_resume, on_destroy;
        event<std::string /*path*/, vec2 /*pos*/> on_drop;

        event<uint8_t /*pointer*/, uint8_t /*button*/, pointer_event_type, vec2 /*pos*/> on_pointer;
        event<char32_t /*utf32_char*/, bool /*down*/> on_char;
        event<control_type, bool /*down*/> on_ctrl;

        void minimize(bool enable);
        bool is_minimized();

        void maximize(bool enable);
        bool is_maximized();

        void fullscreen(bool enable);
        bool is_fullscreen();

        void resize(ivec2 size);
        ivec2 size();

        void rename(std::string name);
        std::string name();

        void close();
    };

} // namespace hut

#ifdef HUT_WAYLAND
    #include "libhut/wayland/window.hpp"
#endif