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

#include <atomic>
#include <charconv>
#include <chrono>
#include <iostream>

#include <unistd.h>
#include <linux/input-event-codes.h>
#include <sys/mman.h>

#include "hut/display.hpp"
#include "hut/window.hpp"

using namespace hut;

void hut::pointer_handle_enter(void *_data, wl_pointer *_pointer, uint32_t _serial, wl_surface *_surface, wl_fixed_t _sx, wl_fixed_t _sy) {
  //std::cout << "pointer_handle_enter " << _pointer << ", " << _surface << ", " << wl_fixed_to_double(_sx) << ", " << wl_fixed_to_double(_sy) << std::endl;
  auto *d = static_cast<display*>(_data);
  auto wit = d->windows_.find(_surface);
  if (_pointer == d->pointer_ && wit != d->windows_.cend()) {
    vec2 coords {wl_fixed_to_int(_sx), wl_fixed_to_int(_sy)};
    auto *w = wit->second;
    assert(w);
    d->last_serial_ = _serial;
    d->pointer_current_ = {_surface, w};
    w->mouse_lastmove_ = coords;
    d->set_cursor(w->current_cursor_type_);
    w->on_mouse.fire(0, MMOVE, coords);
  }
}

void hut::pointer_handle_leave(void *_data, wl_pointer *_pointer, uint32_t _serial, [[maybe_unused]] wl_surface *_surface) {
  //std::cout << "pointer_handle_leave " << _pointer << ", " << _surface << std::endl;
  auto *d = static_cast<display*>(_data);
  if (_pointer == d->pointer_) {
    d->last_serial_ =  _serial;
    d->pointer_current_ = {nullptr, nullptr};
  }
}

void hut::pointer_handle_motion(void *_data, wl_pointer *_pointer, uint32_t, wl_fixed_t _sx, wl_fixed_t _sy) {
  //std::cout << "pointer_handle_motion " << _pointer << ", " << wl_fixed_to_double(_sx) << ", " << wl_fixed_to_double(_sy) << std::endl;
  auto *d = static_cast<display*>(_data);
  if (_pointer == d->pointer_) {
    auto *w = d->pointer_current_.second;
    if (w) {
      vec2 coords {wl_fixed_to_int(_sx), wl_fixed_to_int(_sy)};
      w->mouse_lastmove_ = coords;
      w->on_mouse.fire(0, MMOVE, coords);
    }
  }
}

void hut::pointer_handle_button(void *_data, wl_pointer *_pointer, uint32_t _serial, uint32_t, uint32_t _button, uint32_t _state) {
  //std::cout << "pointer_handle_button " << _pointer << ", " << _button << ", " << _state << std::endl;
  auto *d = static_cast<display*>(_data);
  if (_pointer == d->pointer_) {
    auto *w = d->pointer_current_.second;
    assert(w);
    d->last_serial_ =  _serial;
    w->on_mouse.fire(_button - BTN_MOUSE + 1, _state ? MDOWN : MUP, w->mouse_lastmove_);
  }
}

void hut::pointer_handle_axis(void *_data, wl_pointer *_pointer, uint32_t, uint32_t, wl_fixed_t _value) {
  //std::cout << "pointer_handle_axis " << _pointer << ", " << _axis << ", " << wl_fixed_to_double(_value) << std::endl;
  auto *d = static_cast<display*>(_data);
  if (_pointer == d->pointer_) {
    auto *w = d->pointer_current_.second;
    assert(w);
    w->on_mouse.fire(0, wl_fixed_to_int(_value) > 0 ? MWHEEL_DOWN : MWHEEL_UP, w->mouse_lastmove_);
  }
}

void hut::keyboard_handle_keymap(void *_data, wl_keyboard *_keyboard, uint32_t _format, int _fd, uint32_t _size) {
  //std::cout << "keyboard_handle_keymap " << _keyboard << ", " << _format << ", " << _fd << ", " << _size << std::endl;
  auto *d = static_cast<display*>(_data);
  if (_keyboard != d->keyboard_)
    return;

  void *buf = mmap(nullptr, _size, PROT_READ, MAP_PRIVATE, _fd, 0);
  if (buf == MAP_FAILED)
    throw std::runtime_error(sstream("Failed to mmap keymap: ") << errno);

  if (_format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    throw std::runtime_error(sstream("Unexpected keymap format: ") << _format);

  d->keymap_ = xkb_keymap_new_from_buffer(d->xkb_context_, (const char*)buf, _size - 1, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(buf, _size);
  close(_fd);
  if (!d->keymap_)
    throw std::runtime_error("Failed to compile keymap");

  d->xkb_state_ = xkb_state_new(d->keymap_);
  if (!d->xkb_state_)
    throw std::runtime_error("Failed to create XKB state");

  d->mod_index_alt_ = xkb_keymap_mod_get_index(d->keymap_, XKB_MOD_NAME_ALT);
  d->mod_index_ctrl_ = xkb_keymap_mod_get_index(d->keymap_, XKB_MOD_NAME_CTRL);
  d->mod_index_shift_ = xkb_keymap_mod_get_index(d->keymap_, XKB_MOD_NAME_SHIFT);
}

void hut::keyboard_handle_enter(void *_data, wl_keyboard *_keyboard, uint32_t _serial, wl_surface *_surface, wl_array *) {
  //std::cout << "keyboard_handle_enter " << _keyboard << ", " << _surface << std::endl;
  auto *d = static_cast<display*>(_data);
  auto wit = d->windows_.find(_surface);
  if (_keyboard == d->keyboard_ && wit != d->windows_.cend()) {
    auto *w = wit->second;
    assert(w);
    d->last_serial_ =  _serial;
    d->last_keyboard_enter_serial_ = _serial;
    d->keyboard_current_ = {_surface, w};
    w->on_focus.fire();
  }
}

void hut::keyboard_handle_leave(void *_data, wl_keyboard *_keyboard, uint32_t _serial, wl_surface *_surface) {
  //std::cout << "keyboard_handle_enter " << _keyboard << ", " << _surface << std::endl;
  auto *d = static_cast<display*>(_data);
  auto wit = d->windows_.find(_surface);
  if (_keyboard == d->keyboard_) {
    d->last_serial_ =  _serial;
    d->keyboard_current_ = {nullptr, nullptr};
    if (wit != d->windows_.end())
      wit->second->on_blur.fire();
  }
}

xkb_keysym_t clean_kp_keysyms(xkb_keysym_t _input) {
  switch(_input) {
#define HUT_CLEAN_KP(KEY) case XKB_KEY_KP_##KEY: return XKB_KEY_##KEY;
  case XKB_KEY_KP_Space: return XKB_KEY_space;
  HUT_CLEAN_KP(Tab)
  case XKB_KEY_KP_Enter: return XKB_KEY_Return;
  HUT_CLEAN_KP(F1)
  HUT_CLEAN_KP(F2)
  HUT_CLEAN_KP(F3)
  HUT_CLEAN_KP(F4)
  HUT_CLEAN_KP(Home)
  HUT_CLEAN_KP(Left)
  HUT_CLEAN_KP(Up)
  HUT_CLEAN_KP(Right)
  HUT_CLEAN_KP(Down)
  HUT_CLEAN_KP(Page_Up)
  HUT_CLEAN_KP(Page_Down)
  HUT_CLEAN_KP(End)
  HUT_CLEAN_KP(Begin)
  HUT_CLEAN_KP(Insert)
  HUT_CLEAN_KP(Delete)
  case XKB_KEY_KP_Equal: return XKB_KEY_equal;
  case XKB_KEY_KP_Multiply: return XKB_KEY_asterisk;
  case XKB_KEY_KP_Add: return XKB_KEY_plus;
  case XKB_KEY_KP_Separator: return XKB_KEY_comma;
  case XKB_KEY_KP_Subtract: return XKB_KEY_minus;
  case XKB_KEY_KP_Decimal: return XKB_KEY_period;
  case XKB_KEY_KP_Divide: return XKB_KEY_slash;
  HUT_CLEAN_KP(0)
  HUT_CLEAN_KP(1)
  HUT_CLEAN_KP(2)
  HUT_CLEAN_KP(3)
  HUT_CLEAN_KP(4)
  HUT_CLEAN_KP(5)
  HUT_CLEAN_KP(6)
  HUT_CLEAN_KP(7)
  HUT_CLEAN_KP(8)
  HUT_CLEAN_KP(9)
#undef HUT_CLEAN_KP
    default: return _input;
  }
}

keysym map_keysym(xkb_keysym_t _keysym) {
  switch(_keysym) {
    case XKB_KEY_Tab: return KTAB;
    case XKB_KEY_Alt_L: return KALT_LEFT;
    case XKB_KEY_Alt_R: [[fallthrough]];
    case XKB_KEY_ISO_Level3_Shift: return KALT_RIGHT;
    case XKB_KEY_Control_L: return KCTRL_LEFT;
    case XKB_KEY_Control_R: return KCTRL_RIGHT;
    case XKB_KEY_Shift_L: return KSHIFT_LEFT;
    case XKB_KEY_Shift_R: return KSHIFT_RIGHT;
    case XKB_KEY_Page_Up: return KPAGE_UP;
    case XKB_KEY_Page_Down: return KPAGE_DOWN;
    case XKB_KEY_Up: return KUP;
    case XKB_KEY_Right: return KRIGHT;
    case XKB_KEY_Down: return KDOWN;
    case XKB_KEY_Left: return KLEFT;
    case XKB_KEY_Home: return KHOME;
    case XKB_KEY_End: return KEND;
    case XKB_KEY_Return: return KRETURN;
    case XKB_KEY_BackSpace: return KBACKSPACE;
    case XKB_KEY_Delete: return KDELETE;
    case XKB_KEY_Insert: return KINSER;
    case XKB_KEY_Escape: return KESCAPE;
    case XKB_KEY_F1: return KF1;
    case XKB_KEY_F2: return KF2;
    case XKB_KEY_F3: return KF3;
    case XKB_KEY_F4: return KF4;
    case XKB_KEY_F5: return KF5;
    case XKB_KEY_F6: return KF6;
    case XKB_KEY_F7: return KF7;
    case XKB_KEY_F8: return KF8;
    case XKB_KEY_F9: return KF9;
    case XKB_KEY_F10: return KF10;
    case XKB_KEY_F11: return KF11;
    case XKB_KEY_F12: return KF12;
    default: return static_cast<keysym>(_keysym);
  }
}

void hut::keyboard_handle_key(void *_data, wl_keyboard *_keyboard, uint32_t _serial, uint32_t, uint32_t _key, uint32_t _state) {
  //std::cout << "keyboard_handle_key " << _keyboard << ", " << _key << ", " << _state << std::endl;
  auto *d = static_cast<display*>(_data);
  if (_keyboard == d->keyboard_) {
    auto *w = d->keyboard_current_.second;
    assert(w);
    const auto keycode = xkb_keycode_t(_key + 8); // +8 for evdev scancode to XCB scancode
    const auto ks = xkb_state_key_get_one_sym(d->xkb_state_, keycode);
    const auto cleaned = clean_kp_keysyms(ks);
    const auto mapped = map_keysym(cleaned);
    const bool remapped =  mapped != cleaned && mapped != KENUM_END;
    const auto mods = d->kb_mod_mask_;
    const bool alt = mods & KMOD_ALT;
    const bool ctrl = mods & KMOD_CTRL;

    d->last_serial_ =  _serial;
    w->on_key.fire(mapped, mods, _state != 0);
    if (!remapped && !ctrl && !alt && _state != 0)
      w->on_char.fire(mapped);
  }
}

void hut::keyboard_handle_modifiers(void *_data, wl_keyboard *, uint32_t _serial, uint32_t _mods_depressed, uint32_t _mods_latched, uint32_t _mods_locked, uint32_t _group) {
  //std::cout << "keyboard_handle_modifiers " << _keyboard << ", " << _mods_depressed << ", " << _mods_latched << ", " << _mods_locked << std::endl;
  auto *d = static_cast<display*>(_data);
  d->last_serial_ =  _serial;
  xkb_state_update_mask(d->xkb_state_, _mods_depressed, _mods_latched, _mods_locked, 0, 0, _group);

  modifiers mod_mask = 0;
  if (xkb_state_mod_index_is_active(d->xkb_state_, d->mod_index_alt_, XKB_STATE_MODS_EFFECTIVE))
    mod_mask |= KMOD_ALT;
  if (xkb_state_mod_index_is_active(d->xkb_state_, d->mod_index_ctrl_, XKB_STATE_MODS_EFFECTIVE))
    mod_mask |= KMOD_CTRL;
  if (xkb_state_mod_index_is_active(d->xkb_state_, d->mod_index_shift_, XKB_STATE_MODS_EFFECTIVE))
    mod_mask |= KMOD_SHIFT;
  d->kb_mod_mask_ = mod_mask;
}

void hut::seat_handler(void *_data, wl_seat *_seat, uint32_t _caps) {
  //std::cout << "seat_handler " << _seat << ", " << _caps << std::endl;
  static const wl_keyboard_listener wl_keyboard_listeners = {
      keyboard_handle_keymap, keyboard_handle_enter, keyboard_handle_leave, keyboard_handle_key, keyboard_handle_modifiers, nullptr
  };

  static const wl_pointer_listener wl_pointer_listeners = {
      pointer_handle_enter, pointer_handle_leave, pointer_handle_motion, pointer_handle_button, pointer_handle_axis, nullptr, nullptr, nullptr, nullptr
  };

  auto *d = static_cast<display*>(_data);
  if ((_caps & WL_SEAT_CAPABILITY_POINTER) && !d->pointer_) {
    d->pointer_ = wl_seat_get_pointer(_seat);
    wl_pointer_add_listener(d->pointer_, &wl_pointer_listeners, _data);
  } else if (!(_caps & WL_SEAT_CAPABILITY_POINTER) && d->pointer_) {
    wl_pointer_destroy(d->pointer_);
    d->pointer_ = nullptr;
  }
  if (_caps & WL_SEAT_CAPABILITY_KEYBOARD) {
    d->keyboard_ = wl_seat_get_keyboard(_seat);
    wl_keyboard_add_listener(d->keyboard_, &wl_keyboard_listeners, _data);
  } else if (!(_caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
    wl_keyboard_destroy(d->keyboard_);
    d->keyboard_ = nullptr;
  }
}

void seat_name(void *, wl_seat *, const char *) {
  //std::cout << "seat_name " << _seat << ", " << _name << std::endl;
}

void handle_xdg_ping(void *, struct xdg_wm_base *_shell, uint32_t _serial) {
  xdg_wm_base_pong(_shell, _serial);
}

void hut::registry_handler(void *_data, wl_registry *_registry, uint32_t _id,
                                  const char *_interface, [[maybe_unused]] uint32_t _version) {
  std::cout << "[hut] wayland registry item " << _id << ", " << _interface << ", " << _version << std::endl;
  static const wl_seat_listener wl_seat_listeners = {
      seat_handler, seat_name
  };
  static const xdg_wm_base_listener xdg_wm_base_listeners = {
      handle_xdg_ping,
  };

  auto *d = static_cast<display*>(_data);
  if (strcmp(_interface, wl_compositor_interface.name) == 0) {
    d->compositor_ = static_cast<wl_compositor*>(wl_registry_bind(_registry, _id, &wl_compositor_interface, 1));
  }
  else if (strcmp(_interface, xdg_wm_base_interface.name) == 0) {
    d->xdg_wm_base_ = static_cast<xdg_wm_base*>(wl_registry_bind(_registry, _id, &xdg_wm_base_interface, 1));
    xdg_wm_base_add_listener(d->xdg_wm_base_, &xdg_wm_base_listeners, _data);
  }
  else if (strcmp(_interface, wl_seat_interface.name) == 0) {
    d->seat_ = static_cast<wl_seat*>(wl_registry_bind(_registry, _id, &wl_seat_interface, 1));
    wl_seat_add_listener(d->seat_, &wl_seat_listeners, _data);
  }
  else if (strcmp(_interface, wl_shm_interface.name) == 0) {
    d->shm_ = static_cast<wl_shm*>(wl_registry_bind(_registry, _id, &wl_shm_interface, 1));
  }
  else if (strcmp(_interface, wl_data_device_manager_interface.name) == 0) {
    d->data_device_manager_ = static_cast<wl_data_device_manager*>(wl_registry_bind(_registry, _id, &wl_data_device_manager_interface, 1));
  }
  else if (strcmp(_interface, zxdg_decoration_manager_v1_interface.name) == 0) {
    d->decoration_manager_ = static_cast<zxdg_decoration_manager_v1*>(wl_registry_bind(_registry, _id, &zxdg_decoration_manager_v1_interface, 1));
  }
}

void registry_remove(void *, wl_registry *, uint32_t) {
  //std::cout << "registry_remove " << _id << std::endl;
}

void hut::data_offer_handle_offer(void *_data, wl_data_offer *_offer, const char *_mime_type) {
  auto *d = static_cast<display*>(_data);
  auto format = mime_type_format(_mime_type);
  if (format)
    d->current_data_offer_formats_ |= *format;
}

void hut::data_device_handle_data_offer(void *_data, wl_data_device *_device, wl_data_offer *_offer) {
  static const struct wl_data_offer_listener data_offer_listeners = {
      .offer = data_offer_handle_offer,
  };
  auto *d = static_cast<display*>(_data);
  d->current_data_offer_ = _offer;
  d->current_data_offer_formats_ = 0;
  wl_data_offer_add_listener(_offer, &data_offer_listeners, _data);
}

void hut::data_device_handle_selection(void *_data, wl_data_device *_device, wl_data_offer *_offer) {}

display::display(const char *_app_name, uint32_t _app_version, const char *_name)
  : animate_cursor_ctx_{*this} {
  std::vector<const char *> extensions = {VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
  init_vulkan_instance(_app_name, _app_version, extensions);

  display_ = wl_display_connect(_name);
  if (!display_)
    throw std::runtime_error("couldn't connect to wayland server");

  static const wl_registry_listener reg_listeners = {
      registry_handler, registry_remove
  };
  static const wl_data_device_listener data_device_listeners = {
      .data_offer = data_device_handle_data_offer,
      .selection = data_device_handle_selection,
  };

  xkb_context_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

  registry_ = wl_display_get_registry(display_);
  wl_registry_add_listener(registry_, &reg_listeners, this);
  wl_display_roundtrip(display_);

  if (!compositor_) throw std::runtime_error("couldn't retrieve a wl_compositor in the wayland registry");
  if (!xdg_wm_base_) throw std::runtime_error("couldn't retrieve a xdg_wm_base in the wayland registry");
  if (!seat_) throw std::runtime_error("couldn't retrieve a wl_seat in the wayland registry");

  if (data_device_manager_) {
    data_device_ = wl_data_device_manager_get_data_device(data_device_manager_, seat_);
    wl_data_device_add_listener(data_device_, &data_device_listeners, this);
  }

  auto *dummy = wl_compositor_create_surface(compositor_);
  if (!dummy)
    throw std::runtime_error("couldn't create dummy surface");

  VkWaylandSurfaceCreateInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
  info.display = display_;
  info.surface = dummy;

  auto *func = get_proc<PFN_vkCreateWaylandSurfaceKHR>("vkCreateWaylandSurfaceKHR");
  VkSurfaceKHR dummy_surface;
  VkResult vkr;
  if ((vkr = func(instance_, &info, nullptr, &dummy_surface)) != VK_SUCCESS)
    throw std::runtime_error(sstream("couldn't create vulkan dummy surface, code: ") << vkr);

  init_vulkan_device(dummy_surface);

  vkDestroySurfaceKHR(instance_, dummy_surface, nullptr);
  wl_surface_destroy(dummy);

  FT_Init_FreeType(&ft_library_);

  if (shm_ != nullptr) {
    char *env_cursor_theme = getenv("XCURSOR_THEME");
    char *env_cursor_size = getenv("XCURSOR_SIZE");
    size_t env_cursor_size_length = env_cursor_size == nullptr ? 0 : strlen(env_cursor_size);
    int cursor_size = 24;
    if (env_cursor_size_length > 0)
      std::from_chars(env_cursor_size, env_cursor_size + env_cursor_size_length, cursor_size);
    cursor_theme_ = wl_cursor_theme_load(env_cursor_theme, cursor_size, shm_);
    cursor_surface_ = wl_compositor_create_surface(compositor_);

    animate_cursor_ctx_.thread_ = std::thread(animate_cursor_thread, &animate_cursor_ctx_);
  }
}

display::~display() {
  FT_Done_FreeType(ft_library_);
  destroy_vulkan();

  if (animate_cursor_ctx_.thread_.joinable()) {
    {
      std::scoped_lock lock(animate_cursor_ctx_.mutex_);
      animate_cursor_ctx_.cursor_ = nullptr;
      animate_cursor_ctx_.stop_request_ = true;
    }
    animate_cursor_ctx_.cv_.notify_all();
    animate_cursor_ctx_.thread_.join();
  }

  if (keymap_) xkb_keymap_unref(keymap_);
  if (xkb_state_) xkb_state_unref(xkb_state_);
  if (xkb_context_) xkb_context_unref(xkb_context_);

  if (decoration_manager_) zxdg_decoration_manager_v1_destroy(decoration_manager_);
  if (current_data_offer_) wl_data_offer_destroy(current_data_offer_);
  if (data_device_) wl_data_device_destroy(data_device_);
  if (data_device_manager_) wl_data_device_manager_destroy(data_device_manager_);
  if (cursor_surface_) wl_surface_destroy(cursor_surface_);
  if (cursor_theme_) wl_cursor_theme_destroy(cursor_theme_);
  if (shm_) wl_shm_destroy(shm_);
  if (keyboard_) wl_keyboard_destroy(keyboard_);
  if (pointer_) wl_pointer_destroy(pointer_);
  if (seat_) wl_seat_destroy(seat_);
  if (xdg_wm_base_) xdg_wm_base_destroy(xdg_wm_base_);
  if (compositor_) wl_compositor_destroy(compositor_);
  if (registry_) wl_registry_destroy(registry_);
  if (display_) wl_display_disconnect(display_);
}

void display::flush() {
  wl_display_flush(display_);
}

void display::post_empty_event() {
  wl_callback_destroy(wl_display_sync(display_));
}

int display::dispatch() {
  if (windows_.empty())
    throw std::runtime_error("dispatch called without any window");

  while (loop_) {
    wl_display_dispatch(display_);
    process_posts(display::clock::now());
    if (windows_.empty())
      loop_ = false;
    for (auto wpair : windows_) {
      window *w = wpair.second;
      if (w->invalidated_) {
        w->redraw(display::clock::now());
        w->invalidated_ = false;
      }
    }
  }

  return EXIT_SUCCESS;
}

void display::set_cursor_frame(wl_cursor *_cursor, size_t _frame) {
  assert(_frame < _cursor->image_count);
  wl_cursor_image* image = _cursor->images[_frame];
  wl_buffer *buffer = wl_cursor_image_get_buffer(image);
  wl_surface_attach(cursor_surface_, buffer, 0, 0);
  wl_surface_damage(cursor_surface_, 0, 0, image->width, image->height);
  wl_surface_commit(cursor_surface_);

  wl_pointer_set_cursor(pointer_, last_serial_, cursor_surface_, image->hotspot_x, image->hotspot_y);
}

void display::set_cursor(cursor_type _c) {
  if (cursor_theme_ == nullptr || pointer_ == nullptr)
    return;

  wl_cursor *result = wl_cursor_theme_get_cursor(cursor_theme_, cursor_css_name(_c));
  if (result) {
    if (result->image_count == 1) {
      animate_cursor(nullptr);
      set_cursor_frame(result, 0);
    }
    else {
      animate_cursor(result);
    }
  }
}

void display::animate_cursor(wl_cursor *_cursor) {
  if (_cursor != animate_cursor_ctx_.cursor_) {
    {
      std::scoped_lock lock(animate_cursor_ctx_.mutex_);
      animate_cursor_ctx_.cursor_ = _cursor;
      animate_cursor_ctx_.frame_ = 0;
    }
    animate_cursor_ctx_.cv_.notify_all();
  }
}

void display::animate_cursor_thread(animate_cursor_context *_ctx) {
  std::unique_lock lock(_ctx->mutex_);
  while (!_ctx->stop_request_) {
    if (_ctx->cursor_ == nullptr)
      _ctx->cv_.wait(lock);
    if (_ctx->cursor_) {
      auto *cur_frame = _ctx->cursor_->images[_ctx->frame_];
      _ctx->display_.set_cursor_frame(_ctx->cursor_, _ctx->frame_++);
      if (_ctx->frame_ >= _ctx->cursor_->image_count)
        _ctx->frame_ = 0;
      _ctx->cv_.wait_for(lock, std::chrono::milliseconds(cur_frame->delay));
    }
  }
}
