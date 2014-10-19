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

#include <cstdio>
#include <iostream>

#include <wayland-cursor.h>
#include <linux/input.h>
#include <unistd.h>
#include <sys/mman.h>

#include "libhut/wayland/display.hpp"
#include "libhut/wayland/window.hpp"

namespace hut {

    void display::keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
            uint32_t format, int fd, uint32_t size) {
        struct display *d = (display*)data;
        struct xkb_keymap *keymap;
        struct xkb_state *state;
        char *map_str;

        if (!data) {
            close(fd);
            return;
        }

        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
            close(fd);
            return;
        }

        map_str = (char*)mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
        if (map_str == MAP_FAILED) {
            close(fd);
            return;
        }

        keymap = xkb_keymap_new_from_string(d->xkb_context,
                map_str,
                XKB_KEYMAP_FORMAT_TEXT_V1,
                XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(map_str, size);
        close(fd);

        runtime_assert(keymap != 0, "failed to compile keymap");

        state = xkb_state_new(keymap);
        runtime_assert(state, "xkb_state_new failed");

        xkb_keymap_unref(d->xkb_keymap);
        xkb_state_unref(d->xkb_state);
        d->xkb_keymap = keymap;
        d->xkb_state = state;

        d->xkb_control_mask = (xkb_mod_mask_t)(
                1 << xkb_keymap_mod_get_index(d->xkb_keymap, "Control"));
        d->xkb_alt_mask = (xkb_mod_mask_t)(
                1 << xkb_keymap_mod_get_index(d->xkb_keymap, "Mod1"));
        d->xkb_shift_mask = (xkb_mod_mask_t)(
                1 << xkb_keymap_mod_get_index(d->xkb_keymap, "Shift"));
    }

    void display::keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
            uint32_t serial, struct wl_surface *surface,
            struct wl_array *keys) {
    }

    void display::keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
            uint32_t serial, struct wl_surface *surface) {
    }

    void display::keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
            uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
        struct display *d = (display*)data;

        for(window* win : d->active_wins) {
            switch(key) {
                case KEY_F1: win->on_ctrl.fire(KF1, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_F2: win->on_ctrl.fire(KF2, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_F3: win->on_ctrl.fire(KF3, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_F4: win->on_ctrl.fire(KF4, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_F5: win->on_ctrl.fire(KF5, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_F6: win->on_ctrl.fire(KF6, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_F7: win->on_ctrl.fire(KF7, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_F8: win->on_ctrl.fire(KF8, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_F9: win->on_ctrl.fire(KF9, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;

                case KEY_ESC: win->on_ctrl.fire(KESCAPE, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;

                case KEY_BACKSPACE: win->on_ctrl.fire(KBACKSPACE, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_DELETE: win->on_ctrl.fire(KDELETE, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_INSERT: win->on_ctrl.fire(KINSER, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;

                case KEY_UP: win->on_ctrl.fire(KUP, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_RIGHT: win->on_ctrl.fire(KRIGHT, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_DOWN: win->on_ctrl.fire(KDOWN, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_LEFT: win->on_ctrl.fire(KLEFT, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;

                case KEY_VOLUMEUP: win->on_ctrl.fire(KVOLUME_UP, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_VOLUMEDOWN: win->on_ctrl.fire(KVOLUME_DOWN, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;

                case KEY_PAGEUP: win->on_ctrl.fire(KPAGE_UP, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_PAGEDOWN: win->on_ctrl.fire(KPAGE_DOWN, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;

                case KEY_LEFTALT: win->on_ctrl.fire(KALT_LEFT, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_RIGHTALT: win->on_ctrl.fire(KALT_RIGHT, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;

                case KEY_LEFTCTRL: win->on_ctrl.fire(KCTRL_LEFT, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_RIGHTCTRL: win->on_ctrl.fire(KCTRL_RIGHT, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;

                case KEY_LEFTSHIFT: win->on_ctrl.fire(KSHIFT_LEFT, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;
                case KEY_RIGHTSHIFT: win->on_ctrl.fire(KSHIFT_RIGHT, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0); break;

                default: {
                    const xkb_keysym_t *syms;
                    uint32_t code = key + 8;

                    int num_syms = xkb_state_key_get_syms(d->xkb_state, code, &syms);

                    if (num_syms == 1) {
                        uint32_t ch = xkb_keysym_to_utf32(syms[0]);
                        if(ch != 0)
                            win->on_char.fire(ch, (state & WL_KEYBOARD_KEY_STATE_PRESSED) != 0);
                    }
                }
            }
        }
    }

    void display::keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
            uint32_t serial, uint32_t mods_depressed,
            uint32_t mods_latched, uint32_t mods_locked,
            uint32_t group) {
        struct display *d = (display*)data;
        xkb_mod_mask_t mask;

        /* If we're not using a keymap, then we don't handle PC-style modifiers */
        if (!d->xkb_keymap)
            return;

        xkb_state_update_mask(d->xkb_state, mods_depressed, mods_latched,
                mods_locked, 0, 0, group);
        mask = xkb_state_serialize_mods(d->xkb_state,
                (xkb_state_component)(XKB_STATE_MODS_DEPRESSED |
                        XKB_STATE_MODS_LATCHED));
        /*FIXME
        if (mask & input->xkb.control_mask)
            input->modifiers |= MOD_CONTROL_MASK;
        if (mask & input->xkb.alt_mask)
            input->modifiers |= MOD_ALT_MASK;
        if (mask & input->xkb.shift_mask)
            input->modifiers |= MOD_SHIFT_MASK;*/
    }

    void display::keyboard_handle_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
            int32_t rate, int32_t delay) {
    }

} //namespace hut