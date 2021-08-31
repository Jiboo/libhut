/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste Lepesme github.com/jiboo/libhut
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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

#include <cassert>

#include "hut/utils/math.hpp"

namespace hut {

namespace colors {
constexpr u8vec4 aliceblue            = {240, 248, 255, 255};
constexpr u8vec4 antiquewhite         = {250, 235, 215, 255};
constexpr u8vec4 aqua                 = {0, 255, 255, 255};
constexpr u8vec4 aquamarine           = {127, 255, 212, 255};
constexpr u8vec4 azure                = {240, 255, 255, 255};
constexpr u8vec4 beige                = {245, 245, 220, 255};
constexpr u8vec4 bisque               = {255, 228, 196, 255};
constexpr u8vec4 black                = {0, 0, 0, 255};
constexpr u8vec4 blanchedalmond       = {255, 235, 205, 255};
constexpr u8vec4 blue                 = {0, 0, 255, 255};
constexpr u8vec4 blueviolet           = {138, 43, 226, 255};
constexpr u8vec4 brown                = {165, 42, 42, 255};
constexpr u8vec4 burlywood            = {222, 184, 135, 255};
constexpr u8vec4 cadetblue            = {95, 158, 160, 255};
constexpr u8vec4 chartreuse           = {127, 255, 0, 255};
constexpr u8vec4 chocolate            = {210, 105, 30, 255};
constexpr u8vec4 coral                = {255, 127, 80, 255};
constexpr u8vec4 cornflowerblue       = {100, 149, 237, 255};
constexpr u8vec4 cornsilk             = {255, 248, 220, 255};
constexpr u8vec4 crimson              = {220, 20, 60, 255};
constexpr u8vec4 cyan                 = {0, 255, 255, 255};
constexpr u8vec4 darkblue             = {0, 0, 139, 255};
constexpr u8vec4 darkcyan             = {0, 139, 139, 255};
constexpr u8vec4 darkgoldenrod        = {184, 134, 11, 255};
constexpr u8vec4 darkgray             = {169, 169, 169, 255};
constexpr u8vec4 darkgreen            = {0, 100, 0, 255};
constexpr u8vec4 darkgrey             = {169, 169, 169, 255};
constexpr u8vec4 darkkhaki            = {189, 183, 107, 255};
constexpr u8vec4 darkmagenta          = {139, 0, 139, 255};
constexpr u8vec4 darkolivegreen       = {85, 107, 47, 255};
constexpr u8vec4 darkorange           = {255, 140, 0, 255};
constexpr u8vec4 darkorchid           = {153, 50, 204, 255};
constexpr u8vec4 darkred              = {139, 0, 0, 255};
constexpr u8vec4 darksalmon           = {233, 150, 122, 255};
constexpr u8vec4 darkseagreen         = {143, 188, 143, 255};
constexpr u8vec4 darkslateblue        = {72, 61, 139, 255};
constexpr u8vec4 darkslategray        = {47, 79, 79, 255};
constexpr u8vec4 darkslategrey        = {47, 79, 79, 255};
constexpr u8vec4 darkturquoise        = {0, 206, 209, 255};
constexpr u8vec4 darkviolet           = {148, 0, 211, 255};
constexpr u8vec4 deeppink             = {255, 20, 147, 255};
constexpr u8vec4 deepskyblue          = {0, 191, 255, 255};
constexpr u8vec4 dimgray              = {105, 105, 105, 255};
constexpr u8vec4 dimgrey              = {105, 105, 105, 255};
constexpr u8vec4 dodgerblue           = {30, 144, 255, 255};
constexpr u8vec4 firebrick            = {178, 34, 34, 255};
constexpr u8vec4 floralwhite          = {255, 250, 240, 255};
constexpr u8vec4 forestgreen          = {34, 139, 34, 255};
constexpr u8vec4 fuchsia              = {255, 0, 255, 255};
constexpr u8vec4 gainsboro            = {220, 220, 220, 255};
constexpr u8vec4 ghostwhite           = {248, 248, 255, 255};
constexpr u8vec4 gold                 = {255, 215, 0, 255};
constexpr u8vec4 goldenrod            = {218, 165, 32, 255};
constexpr u8vec4 gray                 = {128, 128, 128, 255};
constexpr u8vec4 green                = {0, 128, 0, 255};
constexpr u8vec4 greenyellow          = {173, 255, 47, 255};
constexpr u8vec4 grey                 = {128, 128, 128, 255};
constexpr u8vec4 honeydew             = {240, 255, 240, 255};
constexpr u8vec4 hotpink              = {255, 105, 180, 255};
constexpr u8vec4 indianred            = {205, 92, 92, 255};
constexpr u8vec4 indigo               = {75, 0, 130, 255};
constexpr u8vec4 ivory                = {255, 255, 240, 255};
constexpr u8vec4 khaki                = {240, 230, 140, 255};
constexpr u8vec4 lavender             = {230, 230, 250, 255};
constexpr u8vec4 lavenderblush        = {255, 240, 245, 255};
constexpr u8vec4 lawngreen            = {124, 252, 0, 255};
constexpr u8vec4 lemonchiffon         = {255, 250, 205, 255};
constexpr u8vec4 lightblue            = {173, 216, 230, 255};
constexpr u8vec4 lightcoral           = {240, 128, 128, 255};
constexpr u8vec4 lightcyan            = {224, 255, 255, 255};
constexpr u8vec4 lightgoldenrodyellow = {250, 250, 210, 255};
constexpr u8vec4 lightgray            = {211, 211, 211, 255};
constexpr u8vec4 lightgreen           = {144, 238, 144, 255};
constexpr u8vec4 lightgrey            = {211, 211, 211, 255};
constexpr u8vec4 lightpink            = {255, 182, 193, 255};
constexpr u8vec4 lightsalmon          = {255, 160, 122, 255};
constexpr u8vec4 lightseagreen        = {32, 178, 170, 255};
constexpr u8vec4 lightskyblue         = {135, 206, 250, 255};
constexpr u8vec4 lightslategray       = {119, 136, 153, 255};
constexpr u8vec4 lightslategrey       = {119, 136, 153, 255};
constexpr u8vec4 lightsteelblue       = {176, 196, 222, 255};
constexpr u8vec4 lightyellow          = {255, 255, 224, 255};
constexpr u8vec4 lime                 = {0, 255, 0, 255};
constexpr u8vec4 limegreen            = {50, 205, 50, 255};
constexpr u8vec4 linen                = {250, 240, 230, 255};
constexpr u8vec4 magenta              = {255, 0, 255, 255};
constexpr u8vec4 maroon               = {128, 0, 0, 255};
constexpr u8vec4 mediumaquamarine     = {102, 205, 170, 255};
constexpr u8vec4 mediumblue           = {0, 0, 205, 255};
constexpr u8vec4 mediumorchid         = {186, 85, 211, 255};
constexpr u8vec4 mediumpurple         = {147, 112, 219, 255};
constexpr u8vec4 mediumseagreen       = {60, 179, 113, 255};
constexpr u8vec4 mediumslateblue      = {123, 104, 238, 255};
constexpr u8vec4 mediumspringgreen    = {0, 250, 154, 255};
constexpr u8vec4 mediumturquoise      = {72, 209, 204, 255};
constexpr u8vec4 mediumvioletred      = {199, 21, 133, 255};
constexpr u8vec4 midnightblue         = {25, 25, 112, 255};
constexpr u8vec4 mintcream            = {245, 255, 250, 255};
constexpr u8vec4 mistyrose            = {255, 228, 225, 255};
constexpr u8vec4 moccasin             = {255, 228, 181, 255};
constexpr u8vec4 navajowhite          = {255, 222, 173, 255};
constexpr u8vec4 navy                 = {0, 0, 128, 255};
constexpr u8vec4 oldlace              = {253, 245, 230, 255};
constexpr u8vec4 olive                = {128, 128, 0, 255};
constexpr u8vec4 olivedrab            = {107, 142, 35, 255};
constexpr u8vec4 orange               = {255, 165, 0, 255};
constexpr u8vec4 orangered            = {255, 69, 0, 255};
constexpr u8vec4 orchid               = {218, 112, 214, 255};
constexpr u8vec4 palegoldenrod        = {238, 232, 170, 255};
constexpr u8vec4 palegreen            = {152, 251, 152, 255};
constexpr u8vec4 paleturquoise        = {175, 238, 238, 255};
constexpr u8vec4 palevioletred        = {219, 112, 147, 255};
constexpr u8vec4 papayawhip           = {255, 239, 213, 255};
constexpr u8vec4 peachpuff            = {255, 218, 185, 255};
constexpr u8vec4 peru                 = {205, 133, 63, 255};
constexpr u8vec4 pink                 = {255, 192, 203, 255};
constexpr u8vec4 plum                 = {221, 160, 221, 255};
constexpr u8vec4 powderblue           = {176, 224, 230, 255};
constexpr u8vec4 purple               = {128, 0, 128, 255};
constexpr u8vec4 red                  = {255, 0, 0, 255};
constexpr u8vec4 rosybrown            = {188, 143, 143, 255};
constexpr u8vec4 royalblue            = {65, 105, 225, 255};
constexpr u8vec4 saddlebrown          = {139, 69, 19, 255};
constexpr u8vec4 salmon               = {250, 128, 114, 255};
constexpr u8vec4 sandybrown           = {244, 164, 96, 255};
constexpr u8vec4 seagreen             = {46, 139, 87, 255};
constexpr u8vec4 seashell             = {255, 245, 238, 255};
constexpr u8vec4 sienna               = {160, 82, 45, 255};
constexpr u8vec4 silver               = {192, 192, 192, 255};
constexpr u8vec4 skyblue              = {135, 206, 235, 255};
constexpr u8vec4 slateblue            = {106, 90, 205, 255};
constexpr u8vec4 slategray            = {112, 128, 144, 255};
constexpr u8vec4 slategrey            = {112, 128, 144, 255};
constexpr u8vec4 snow                 = {255, 250, 250, 255};
constexpr u8vec4 springgreen          = {0, 255, 127, 255};
constexpr u8vec4 steelblue            = {70, 130, 180, 255};
constexpr u8vec4 tan                  = {210, 180, 140, 255};
constexpr u8vec4 teal                 = {0, 128, 128, 255};
constexpr u8vec4 thistle              = {216, 191, 216, 255};
constexpr u8vec4 tomato               = {255, 99, 71, 255};
constexpr u8vec4 turquoise            = {64, 224, 208, 255};
constexpr u8vec4 violet               = {238, 130, 238, 255};
constexpr u8vec4 wheat                = {245, 222, 179, 255};
constexpr u8vec4 white                = {255, 255, 255, 255};
constexpr u8vec4 whitesmoke           = {245, 245, 245, 255};
constexpr u8vec4 yellow               = {255, 255, 0, 255};
constexpr u8vec4 yellowgreen          = {154, 205, 50, 255};
};  // namespace colors

}  // namespace hut
