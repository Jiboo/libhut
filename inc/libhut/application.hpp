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

#include <memory>

#include "libhut/event.hpp"
#include "libhut/display.hpp"
#include "libhut/window.hpp"

namespace hut {

    /* Your app, define your implementation with the <HUT_MAIN> macro.
     * Upon app launch, an instance of the class defined by <HUT_MAIN> will be created
     * and <entry> will get called.
     */
    class base_application {
    public:
        /* Gets the default display (as defined by DISPLAY on X) */
        static display &auto_display();

        //TODO Some way to list and get shared_ptr to other displays

        bool stop = false;

    protected:
        /* Your application entry point, create widgets and add the root of your hierarchy to the window */
        virtual int entry(int argc, char **argv, window& main) {
            throw std::runtime_error("Entry not overloaded, define HUT_MAIN with your application class.");
            return EXIT_FAILURE;
        }

        virtual bool loop() = 0;
    };

} //namespace hut

#ifdef HUT_WAYLAND

#include "libhut/wayland/application.hpp"

#endif