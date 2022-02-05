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

namespace hut::colors {

constexpr u8vec4 ALICE_BLUE             = {240, 248, 255, 255};
constexpr u8vec4 ANTIQUE_WHITE          = {250, 235, 215, 255};
constexpr u8vec4 AQUA                   = {0, 255, 255, 255};
constexpr u8vec4 AQUAMARINE             = {127, 255, 212, 255};
constexpr u8vec4 AZURE                  = {240, 255, 255, 255};
constexpr u8vec4 BEIGE                  = {245, 245, 220, 255};
constexpr u8vec4 BISQUE                 = {255, 228, 196, 255};
constexpr u8vec4 BLACK                  = {0, 0, 0, 255};
constexpr u8vec4 BLANCHED_ALMOND        = {255, 235, 205, 255};
constexpr u8vec4 BLUE                   = {0, 0, 255, 255};
constexpr u8vec4 BLUE_VIOLET            = {138, 43, 226, 255};
constexpr u8vec4 BROWN                  = {165, 42, 42, 255};
constexpr u8vec4 BURLY_WOOD             = {222, 184, 135, 255};
constexpr u8vec4 CADET_BLUE             = {95, 158, 160, 255};
constexpr u8vec4 CHARTREUSE             = {127, 255, 0, 255};
constexpr u8vec4 CHOCOLATE              = {210, 105, 30, 255};
constexpr u8vec4 CORAL                  = {255, 127, 80, 255};
constexpr u8vec4 CORNFLOWER_BLUE        = {100, 149, 237, 255};
constexpr u8vec4 CORN_SILK              = {255, 248, 220, 255};
constexpr u8vec4 CRIMSON                = {220, 20, 60, 255};
constexpr u8vec4 CYAN                   = {0, 255, 255, 255};
constexpr u8vec4 DARK_BLUE              = {0, 0, 139, 255};
constexpr u8vec4 DARK_CYAN              = {0, 139, 139, 255};
constexpr u8vec4 DARK_GOLDENROD         = {184, 134, 11, 255};
constexpr u8vec4 DARK_GRAY              = {169, 169, 169, 255};
constexpr u8vec4 DARK_GREEN             = {0, 100, 0, 255};
constexpr u8vec4 DARK_GREY              = {169, 169, 169, 255};
constexpr u8vec4 DARK_KHAKI             = {189, 183, 107, 255};
constexpr u8vec4 DARK_MAGENTA           = {139, 0, 139, 255};
constexpr u8vec4 DARK_OLIVE_GREEN       = {85, 107, 47, 255};
constexpr u8vec4 DARK_ORANGE            = {255, 140, 0, 255};
constexpr u8vec4 DARK_ORCHID            = {153, 50, 204, 255};
constexpr u8vec4 DARK_RED               = {139, 0, 0, 255};
constexpr u8vec4 DARK_SALMON            = {233, 150, 122, 255};
constexpr u8vec4 DARK_SEA_GREEN         = {143, 188, 143, 255};
constexpr u8vec4 DARK_SLATE_BLUE        = {72, 61, 139, 255};
constexpr u8vec4 DARK_SLATE_GRAY        = {47, 79, 79, 255};
constexpr u8vec4 DARK_SLATE_GREY        = {47, 79, 79, 255};
constexpr u8vec4 DARK_TURQUOISE         = {0, 206, 209, 255};
constexpr u8vec4 DARK_VIOLET            = {148, 0, 211, 255};
constexpr u8vec4 DEEP_PINK              = {255, 20, 147, 255};
constexpr u8vec4 DEEP_SKY_BLUE          = {0, 191, 255, 255};
constexpr u8vec4 DIM_GRAY               = {105, 105, 105, 255};
constexpr u8vec4 DIM_GREY               = {105, 105, 105, 255};
constexpr u8vec4 DODGER_BLUE            = {30, 144, 255, 255};
constexpr u8vec4 FIREBRICK              = {178, 34, 34, 255};
constexpr u8vec4 FLORAL_WHITE           = {255, 250, 240, 255};
constexpr u8vec4 FOREST_GREEN           = {34, 139, 34, 255};
constexpr u8vec4 FUCHSIA                = {255, 0, 255, 255};
constexpr u8vec4 GAINSBORO              = {220, 220, 220, 255};
constexpr u8vec4 GHOST_WHITE            = {248, 248, 255, 255};
constexpr u8vec4 GOLD                   = {255, 215, 0, 255};
constexpr u8vec4 GOLDENROD              = {218, 165, 32, 255};
constexpr u8vec4 GRAY                   = {128, 128, 128, 255};
constexpr u8vec4 GREEN                  = {0, 128, 0, 255};
constexpr u8vec4 GREEN_YELLOW           = {173, 255, 47, 255};
constexpr u8vec4 GREY                   = {128, 128, 128, 255};
constexpr u8vec4 HONEYDEW               = {240, 255, 240, 255};
constexpr u8vec4 HOT_PINK               = {255, 105, 180, 255};
constexpr u8vec4 INDIAN_RED             = {205, 92, 92, 255};
constexpr u8vec4 INDIGO                 = {75, 0, 130, 255};
constexpr u8vec4 IVORY                  = {255, 255, 240, 255};
constexpr u8vec4 KHAKI                  = {240, 230, 140, 255};
constexpr u8vec4 LAVENDER               = {230, 230, 250, 255};
constexpr u8vec4 LAVENDER_BLUSH         = {255, 240, 245, 255};
constexpr u8vec4 LAWN_GREEN             = {124, 252, 0, 255};
constexpr u8vec4 LEMON_CHIFFON          = {255, 250, 205, 255};
constexpr u8vec4 LIGHT_BLUE             = {173, 216, 230, 255};
constexpr u8vec4 LIGHT_CORAL            = {240, 128, 128, 255};
constexpr u8vec4 LIGHT_CYAN             = {224, 255, 255, 255};
constexpr u8vec4 LIGHT_GOLDENROD_YELLOW = {250, 250, 210, 255};
constexpr u8vec4 LIGHT_GRAY             = {211, 211, 211, 255};
constexpr u8vec4 LIGHT_GREEN            = {144, 238, 144, 255};
constexpr u8vec4 LIGHT_GREY             = {211, 211, 211, 255};
constexpr u8vec4 LIGHT_PINK             = {255, 182, 193, 255};
constexpr u8vec4 LIGHT_SALMON           = {255, 160, 122, 255};
constexpr u8vec4 LIGHT_SEA_GREEN        = {32, 178, 170, 255};
constexpr u8vec4 LIGHT_SKY_BLUE         = {135, 206, 250, 255};
constexpr u8vec4 LIGHT_SLATE_GRAY       = {119, 136, 153, 255};
constexpr u8vec4 LIGHT_SLATE_GREY       = {119, 136, 153, 255};
constexpr u8vec4 LIGHT_STEEL_BLUE       = {176, 196, 222, 255};
constexpr u8vec4 LIGHT_YELLOW           = {255, 255, 224, 255};
constexpr u8vec4 LIME                   = {0, 255, 0, 255};
constexpr u8vec4 LIME_GREEN             = {50, 205, 50, 255};
constexpr u8vec4 LINEN                  = {250, 240, 230, 255};
constexpr u8vec4 MAGENTA                = {255, 0, 255, 255};
constexpr u8vec4 MAROON                 = {128, 0, 0, 255};
constexpr u8vec4 MEDIUM_AQUAMARINE      = {102, 205, 170, 255};
constexpr u8vec4 MEDIUM_BLUE            = {0, 0, 205, 255};
constexpr u8vec4 MEDIUM_ORCHID          = {186, 85, 211, 255};
constexpr u8vec4 MEDIUM_PURPLE          = {147, 112, 219, 255};
constexpr u8vec4 MEDIUM_SEA_GREEN       = {60, 179, 113, 255};
constexpr u8vec4 MEDIUM_SLATE_BLUE      = {123, 104, 238, 255};
constexpr u8vec4 MEDIUM_SPRING_GREEN    = {0, 250, 154, 255};
constexpr u8vec4 MEDIUM_TURQUOISE       = {72, 209, 204, 255};
constexpr u8vec4 MEDIUM_VIOLET_RED      = {199, 21, 133, 255};
constexpr u8vec4 MIDNIGHT_BLUE          = {25, 25, 112, 255};
constexpr u8vec4 MINT_CREAM             = {245, 255, 250, 255};
constexpr u8vec4 MISTY_ROSE             = {255, 228, 225, 255};
constexpr u8vec4 MOCCASIN               = {255, 228, 181, 255};
constexpr u8vec4 NAVAJO_WHITE           = {255, 222, 173, 255};
constexpr u8vec4 NAVY                   = {0, 0, 128, 255};
constexpr u8vec4 OLD_LACE               = {253, 245, 230, 255};
constexpr u8vec4 OLIVE                  = {128, 128, 0, 255};
constexpr u8vec4 OLIVE_DRAB             = {107, 142, 35, 255};
constexpr u8vec4 ORANGE                 = {255, 165, 0, 255};
constexpr u8vec4 ORANGE_RED             = {255, 69, 0, 255};
constexpr u8vec4 ORCHID                 = {218, 112, 214, 255};
constexpr u8vec4 PALE_GOLDENROD         = {238, 232, 170, 255};
constexpr u8vec4 PALE_GREEN             = {152, 251, 152, 255};
constexpr u8vec4 PALE_TURQUOISE         = {175, 238, 238, 255};
constexpr u8vec4 PALE_VIOLET_RED        = {219, 112, 147, 255};
constexpr u8vec4 PAPAYA_WHIP            = {255, 239, 213, 255};
constexpr u8vec4 PEACH_PUFF             = {255, 218, 185, 255};
constexpr u8vec4 PERU                   = {205, 133, 63, 255};
constexpr u8vec4 PINK                   = {255, 192, 203, 255};
constexpr u8vec4 PLUM                   = {221, 160, 221, 255};
constexpr u8vec4 POWDER_BLUE            = {176, 224, 230, 255};
constexpr u8vec4 PURPLE                 = {128, 0, 128, 255};
constexpr u8vec4 RED                    = {255, 0, 0, 255};
constexpr u8vec4 ROSY_BROWN             = {188, 143, 143, 255};
constexpr u8vec4 ROYAL_BLUE             = {65, 105, 225, 255};
constexpr u8vec4 SADDLE_BROWN           = {139, 69, 19, 255};
constexpr u8vec4 SALMON                 = {250, 128, 114, 255};
constexpr u8vec4 SANDY_BROWN            = {244, 164, 96, 255};
constexpr u8vec4 SEA_GREEN              = {46, 139, 87, 255};
constexpr u8vec4 SEASHELL               = {255, 245, 238, 255};
constexpr u8vec4 SIENNA                 = {160, 82, 45, 255};
constexpr u8vec4 SILVER                 = {192, 192, 192, 255};
constexpr u8vec4 SKY_BLUE               = {135, 206, 235, 255};
constexpr u8vec4 SLATE_BLUE             = {106, 90, 205, 255};
constexpr u8vec4 SLATE_GRAY             = {112, 128, 144, 255};
constexpr u8vec4 SLATE_GREY             = {112, 128, 144, 255};
constexpr u8vec4 SNOW                   = {255, 250, 250, 255};
constexpr u8vec4 SPRING_GREEN           = {0, 255, 127, 255};
constexpr u8vec4 STEEL_BLUE             = {70, 130, 180, 255};
constexpr u8vec4 TAN                    = {210, 180, 140, 255};
constexpr u8vec4 TEAL                   = {0, 128, 128, 255};
constexpr u8vec4 THISTLE                = {216, 191, 216, 255};
constexpr u8vec4 TOMATO                 = {255, 99, 71, 255};
constexpr u8vec4 TURQUOISE              = {64, 224, 208, 255};
constexpr u8vec4 VIOLET                 = {238, 130, 238, 255};
constexpr u8vec4 WHEAT                  = {245, 222, 179, 255};
constexpr u8vec4 WHITE                  = {255, 255, 255, 255};
constexpr u8vec4 WHITE_SMOKE            = {245, 245, 245, 255};
constexpr u8vec4 YELLOW                 = {255, 255, 0, 255};
constexpr u8vec4 YELLOW_GREEN           = {154, 205, 50, 255};

}  // namespace hut::colors
