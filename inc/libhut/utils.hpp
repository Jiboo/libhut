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
#include <stdexcept>

namespace hut {

    inline void runtime_assert(bool test, const std::string &error) {
        if (!test) {
            throw std::runtime_error(error);
        }
    }

    inline std::string to_utf8(char32_t ch) {
        //Credits to libcaca
        static const uint8_t mark[7] = {
                0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
        };

        if(ch < 0x80) {
            return std::string {(char)ch};
        }
        else {
            char result[4];
            size_t bytes = (ch < 0x800) ? 2 : (ch < 0x10000) ? 3 : 4;

            auto it = result + bytes;

            switch(bytes)
            {
                case 4: *--it = (char)((ch | 0x80) & 0xbf); ch >>= 6;
                case 3: *--it = (char)((ch | 0x80) & 0xbf); ch >>= 6;
                case 2: *--it = (char)((ch | 0x80) & 0xbf); ch >>= 6;
            }
            *--it = (char)(ch | mark[bytes]);

            return std::string {result, bytes};
        }

        return std::string {};
    }

} //namespace hut
