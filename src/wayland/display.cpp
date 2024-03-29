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

#include "hut/display.hpp"

#include <charconv>
#include <iostream>

#include <linux/input-event-codes.h>
#include <sys/mman.h>
#include <unistd.h>

#include "hut/utils/profiling.hpp"
#include "hut/utils/vulkan.hpp"

#include "hut/buffer.hpp"
#include "hut/window.hpp"

namespace hut {

void display::pointer_handle_enter(void *_data, wl_pointer *_pointer, u32 _serial, wl_surface *_surface, wl_fixed_t _sx,
                                   wl_fixed_t _sy) {
  //std::cout << "pointer_handle_enter " << _pointer << ", " << _surface << ", " << wl_fixed_to_double(_sx) << ", " << wl_fixed_to_double(_sy) << std::endl;
  auto *d   = static_cast<display *>(_data);
  auto  wit = d->windows_.find(_surface);
  if (_pointer == d->pointer_ && wit != d->windows_.cend()) {
    vec2  coords{wl_fixed_to_int(_sx), wl_fixed_to_int(_sy)};
    auto *w = wit->second;
    assert(w);
    d->pointer_current_         = {_surface, w};
    w->mouse_lastmove_          = coords;
    d->last_serial_             = _serial;
    d->last_mouse_enter_serial_ = _serial;
    d->cursor(w->current_cursor_type_, w->scale_);
    HUT_PROFILE_EVENT_NAMED(w, on_mouse_, ("button", "mouse_event_type", "coords"), 0, MMOVE, coords);
  }
}

void display::pointer_handle_leave(void *_data, wl_pointer *_pointer, u32 _serial, wl_surface *_surface) {
  //std::cout << "pointer_handle_leave " << _pointer << ", " << _surface << std::endl;
  auto *d = static_cast<display *>(_data);
  if (_pointer == d->pointer_) {
    d->pointer_current_ = {nullptr, nullptr};
  }
}

void display::pointer_handle_motion(void *_data, wl_pointer *_pointer, u32 _time, wl_fixed_t _sx, wl_fixed_t _sy) {
  //std::cout << "pointer_handle_motion " << _pointer << ", " << wl_fixed_to_double(_sx) << ", " << wl_fixed_to_double(_sy) << std::endl;
  auto *d = static_cast<display *>(_data);
  if (_pointer == d->pointer_) {
    auto *w = d->pointer_current_.second;
    if (w != nullptr) {
      vec2 coords{wl_fixed_to_int(_sx), wl_fixed_to_int(_sy)};
      w->mouse_lastmove_ = coords;

      HUT_PROFILE_EVENT_NAMED(w, on_mouse_, ("button", "mouse_event_type", "coords"), 0, MMOVE, coords);
#if defined(HUT_ENABLE_TIME_EVENTS)
      HUT_PROFILE_EVENT(w, on_time_, _time);
#endif
    }
  }
}

void display::pointer_handle_button(void *_data, wl_pointer *_pointer, u32 _serial, u32 _time, u32 _button,
                                    u32 _state) {
  //std::cout << "pointer_handle_button " << _pointer << ", " << _button << ", " << _state << std::endl;
  auto *d = static_cast<display *>(_data);
  if (_pointer == d->pointer_) {
    auto *w = d->pointer_current_.second;
    if (w != nullptr) {
      d->last_mouse_click_serial_ = _serial;
      d->last_serial_             = _serial;
      HUT_PROFILE_EVENT_NAMED(w, on_mouse_, ("button", "mouse_event_type", "coords"), _button - BTN_MOUSE + 1,
                              _state ? MDOWN : MUP, w->mouse_lastmove_);
    }
  }
}

void display::pointer_handle_axis(void *_data, wl_pointer *_pointer, u32 _time, u32 _axis, wl_fixed_t _value) {
  //std::cout << "pointer_handle_axis " << _pointer << ", " << _axis << ", " << wl_fixed_to_double(_value) << std::endl;
  auto *d = static_cast<display *>(_data);
  if (_pointer == d->pointer_) {
    auto *w = d->pointer_current_.second;
    assert(w);

    HUT_PROFILE_EVENT_NAMED(w, on_mouse_, ("button", "mouse_event_type", "coords"), 0,
                            wl_fixed_to_int(_value) > 0 ? MWHEEL_DOWN : MWHEEL_UP, w->mouse_lastmove_);
  }
}

void display::pointer_handle_frame(void *_data, wl_pointer *_pointer) {
  /*std::cout << "pointer_handle_frame " << _pointer << std::endl;
  auto *d = static_cast<display*>(_data);*/
}

void display::pointer_handle_axis_source(void *_data, wl_pointer *_pointer, u32 _source) {
  /*std::cout << "pointer_handle_axis_source " << _pointer << ", " << _source << std::endl;
  auto *d = static_cast<display*>(_data);*/
}

void display::pointer_handle_axis_stop(void *_data, wl_pointer *_pointer, u32 _time, u32 _axis) {
  /*std::cout << "pointer_handle_axis_stop " << _pointer << ", " << _time << ", " << _axis << std::endl;
  auto *d = static_cast<display*>(_data);*/
}

void display::pointer_handle_axis_discrete(void *_data, wl_pointer *_pointer, u32 _axis, i32 _discrete) {
  /*std::cout << "pointer_handle_axis_discrete " << _pointer << ", " << _axis << ", " << _discrete << std::endl;
  auto *d = static_cast<display*>(_data);*/
}

keysym display::map_xkb_keysym(xkb_keysym_t _in) {
  switch (_in) {
#define HUT_MAP_KEYSYM(FORMAT_LINUX, FORMAT_X11, FORMAT_IMGUI)                                                         \
  case XKB_KEY_##FORMAT_X11:                                                                                           \
    return KSYM_##FORMAT_LINUX;
#include "hut/keysyms.inc"
#undef HUT_MAP_KEYSYM
    default: return KSYM_INVALID;
  }
}

char32_t display::keycode_idle_char(keycode _in) const {
  const auto xkb_keysym = xkb_state_key_get_one_sym(xkb_state_empty_, _in);
  const auto xkb_upper  = xkb_keysym_to_upper(xkb_keysym);
  return xkb_keysym_to_utf32(xkb_upper);
}

char *display::keycode_name(std::span<char> _out, keycode _in) const {
  const auto xkb_keysym = xkb_state_key_get_one_sym(xkb_state_empty_, _in);
  const auto xkb_upper  = xkb_keysym_to_upper(xkb_keysym);
  auto       result     = xkb_keysym_get_name(xkb_upper, _out.data(), _out.size());
  return _out.data() + std::max(0, result);
}

void display::keyboard_handle_keymap(void *_data, wl_keyboard *_keyboard, u32 _format, int _fd, u32 _size) {
  //std::cout << "keyboard_handle_keymap " << _keyboard << ", " << _format << ", " << _fd << ", " << _size << std::endl;
  auto *d = static_cast<display *>(_data);
  if (_keyboard != d->keyboard_)
    return;

  void *buf = mmap(nullptr, _size, PROT_READ, MAP_PRIVATE, _fd, 0);
  if (buf == MAP_FAILED)
    throw std::runtime_error(sstream("Failed to mmap keymap: ") << errno);

  if (_format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    throw std::runtime_error(sstream("Unexpected keymap format: ") << _format);

  d->keymap_ = xkb_keymap_new_from_buffer(d->xkb_context_, (const char *)buf, _size - 1, XKB_KEYMAP_FORMAT_TEXT_V1,
                                          XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(buf, _size);
  close(_fd);
  if (d->keymap_ == nullptr)
    throw std::runtime_error("Failed to compile keymap");

  d->xkb_state_       = xkb_state_new(d->keymap_);
  d->xkb_state_empty_ = xkb_state_new(d->keymap_);
  if ((d->xkb_state_ == nullptr) || (d->xkb_state_empty_ == nullptr))
    throw std::runtime_error("Failed to create XKB states");

  d->mod_index_alt_   = xkb_keymap_mod_get_index(d->keymap_, XKB_MOD_NAME_ALT);
  d->mod_index_ctrl_  = xkb_keymap_mod_get_index(d->keymap_, XKB_MOD_NAME_CTRL);
  d->mod_index_shift_ = xkb_keymap_mod_get_index(d->keymap_, XKB_MOD_NAME_SHIFT);
}

void display::keyboard_handle_enter(void *_data, wl_keyboard *_keyboard, u32 _serial, wl_surface *_surface,
                                    wl_array * /*unused*/) {
  //std::cout << "keyboard_handle_enter " << _keyboard << ", " << _surface << std::endl;
  auto *d   = static_cast<display *>(_data);
  auto  wit = d->windows_.find(_surface);
  if (_keyboard == d->keyboard_ && wit != d->windows_.cend()) {
    auto *w = wit->second;
    assert(w);
    d->keyboard_current_ = {_surface, w};
    d->last_serial_      = _serial;
    HUT_PROFILE_EVENT(w, on_focus_, true);
  }
}

void display::keyboard_handle_leave(void *_data, wl_keyboard *_keyboard, u32 _serial, wl_surface *_surface) {
  //std::cout << "keyboard_handle_enter " << _keyboard << ", " << _surface << std::endl;
  auto *d   = static_cast<display *>(_data);
  auto  wit = d->windows_.find(_surface);
  if (_keyboard == d->keyboard_) {
    d->keyboard_current_ = {nullptr, nullptr};
    if (wit != d->windows_.end()) {
      d->last_serial_ = _serial;
      HUT_PROFILE_EVENT(wit->second, on_focus_, false);
      d->keyboard_repeat(0);
    }
  }
}

void display::keyboard_handle_key(void *_data, wl_keyboard *_keyboard, u32 _serial, u32 _time, u32 _key, u32 _state) {
  //std::cout << "keyboard_handle_key " << _keyboard << ", " << _key << ", " << _state << std::endl;
  auto *d = static_cast<display *>(_data);
  if (_keyboard == d->keyboard_) {
    auto *w = d->keyboard_current_.second;
    assert(w);
    const auto kcode      = xkb_keycode_t(_key + 8);
    const auto ksym       = xkb_state_key_get_one_sym(d->xkb_state_, kcode);
    const auto upper_ksym = xkb_keysym_to_upper(ksym);
    const auto hut_ksym   = display::map_xkb_keysym(upper_ksym);

    d->last_serial_ = _serial;
    HUT_PROFILE_EVENT_NAMED(w, on_key_, ("kcode", "ksym", "down"), kcode, hut_ksym, _state != 0);
    if (_state != 0) {
      const char32_t utf32     = xkb_keysym_to_utf32(ksym);
      const auto     valid     = utf32 != 0;
      const auto     printable = utf32 > 0x7f || (isprint(int(utf32)) != 0);
      if (valid && printable) {
        d->keyboard_repeat(utf32);
        HUT_PROFILE_EVENT(w, on_char_, (u32)utf32);
      }
    } else {
      d->keyboard_repeat(0);
    }
  }
}

void display::keyboard_handle_repeat_info(void *_data, wl_keyboard *_keyboard, i32 _rate, i32 _delay) {
  //std::cout << "keyboard_handle_repeat_info " << _rate << ", " << _delay << std::endl;
  auto *d                        = static_cast<display *>(_data);
  d->keyboard_repeat_ctx_.delay_ = std::chrono::milliseconds(_delay);
  d->keyboard_repeat_ctx_.sleep_ = std::chrono::milliseconds(1000 / _rate);
}

void display::keyboard_handle_modifiers(void *_data, wl_keyboard *_keyboard, u32 _serial, u32 _mods_depressed,
                                        u32 _mods_latched, u32 _mods_locked, u32 _group) {
  //std::cout << "keyboard_handle_modifiers " << _mods_depressed << ", " << _mods_latched << ", " << _mods_locked << std::endl;
  auto *d = static_cast<display *>(_data);
  xkb_state_update_mask(d->xkb_state_, _mods_depressed, _mods_latched, _mods_locked, 0, 0, _group);

  modifiers mod_mask{};
  if (xkb_state_mod_index_is_active(d->xkb_state_, d->mod_index_alt_, XKB_STATE_MODS_EFFECTIVE) != 0)
    mod_mask |= KMOD_ALT;
  if (xkb_state_mod_index_is_active(d->xkb_state_, d->mod_index_ctrl_, XKB_STATE_MODS_EFFECTIVE) != 0)
    mod_mask |= KMOD_CTRL;
  if (xkb_state_mod_index_is_active(d->xkb_state_, d->mod_index_shift_, XKB_STATE_MODS_EFFECTIVE) != 0)
    mod_mask |= KMOD_SHIFT;

  auto *w = d->keyboard_current_.second;
  if (_keyboard == d->keyboard_ && (w != nullptr)) {
    d->last_serial_ = _serial;
    HUT_PROFILE_EVENT(w, on_kmods_, mod_mask);
  }
}

void display::seat_handler(void *_data, wl_seat *_seat, u32 _caps) {
  //std::cout << "seat_handler " << _seat << ", " << _caps << std::endl;
  const static wl_pointer_listener S_POINTER_LISTENERS
      = {pointer_handle_enter,       pointer_handle_leave,     pointer_handle_motion,
         pointer_handle_button,      pointer_handle_axis,      pointer_handle_frame,
         pointer_handle_axis_source, pointer_handle_axis_stop, pointer_handle_axis_discrete};

  const static wl_keyboard_listener S_KEYBOARD_LISTENERS
      = {keyboard_handle_keymap, keyboard_handle_enter,     keyboard_handle_leave,
         keyboard_handle_key,    keyboard_handle_modifiers, keyboard_handle_repeat_info};

  auto *d = static_cast<display *>(_data);
  if (((_caps & WL_SEAT_CAPABILITY_POINTER) != 0u) && (d->pointer_ == nullptr)) {
    d->pointer_ = wl_seat_get_pointer(_seat);
    wl_pointer_add_listener(d->pointer_, &S_POINTER_LISTENERS, _data);
  } else if (((_caps & WL_SEAT_CAPABILITY_POINTER) == 0u) && (d->pointer_ != nullptr)) {
    wl_pointer_destroy(d->pointer_);
    d->pointer_ = nullptr;
  }

  if ((_caps & WL_SEAT_CAPABILITY_KEYBOARD) != 0u) {
    d->keyboard_ = wl_seat_get_keyboard(_seat);
    wl_keyboard_add_listener(d->keyboard_, &S_KEYBOARD_LISTENERS, _data);
  } else if ((_caps & WL_SEAT_CAPABILITY_KEYBOARD) == 0u) {
    wl_keyboard_destroy(d->keyboard_);
    d->keyboard_ = nullptr;
  }
}

void seat_name(void * /*unused*/, wl_seat * /*unused*/, const char * /*unused*/) {
  //std::cout << "seat_name " << _seat << ", " << _name << std::endl;
}

void handle_xdg_ping(void * /*unused*/, struct xdg_wm_base *_shell, u32 _serial) {
  xdg_wm_base_pong(_shell, _serial);
}

void display::output_geometry(void *_data, wl_output *_output, int32_t _x, int32_t _y, int32_t _pwidth,
                              int32_t _pheight, int32_t _subpixel, const char *_maker, const char *_model,
                              int32_t _transform) {
#if defined(HUT_ENABLE_VALIDATION_DEBUG) && 0
  std::cout << "[hut] output_geometry " << _data << ", " << _output << ", x: " << _x << ", y: " << _y
            << ", _pwidth: " << _pwidth << ", _pheight: " << _pheight << ", _subpixel: " << _subpixel
            << ", _transform: " << _transform << ", maker: " << _maker << ", model: " << _model << std::endl;
#endif
}
void display::output_mode(void *_data, wl_output *_output, uint32_t _flags, int32_t _width, int32_t _height,
                          int32_t _refresh) {
#if defined(HUT_ENABLE_VALIDATION_DEBUG) && 0
  std::cout << "[hut] output_mode " << _output << ", " << _flags << ", " << _width << ", " << _height << ", "
            << _refresh << std::endl;
#endif
}
void display::output_scale(void *_data, wl_output *_output, int32_t _scale) {
#if defined(HUT_ENABLE_VALIDATION_DEBUG) && 0
  std::cout << "[hut] output_scale " << _scale << std::endl;
#endif
  auto *d                    = static_cast<display *>(_data);
  d->outputs_scale_[_output] = _scale;
}
void output_done(void *_data, struct wl_output *_output) {
#if defined(HUT_ENABLE_VALIDATION_DEBUG) && 0
  std::cout << "[hut] output_done " << std::endl;
#endif
}

void display::registry_handler(void *_data, wl_registry *_registry, u32 _id, const char *_interface, u32 _version) {
#ifdef HUT_ENABLE_VALIDATION_DEBUG
  std::cout << "[hut] wayland registry item " << _id << ", " << _interface << ", " << _version << std::endl;
#endif

  const static wl_seat_listener     S_SEAT_LISTENERS        = {seat_handler, seat_name};
  const static xdg_wm_base_listener S_XDG_WM_BASE_LISTENERS = {handle_xdg_ping};
  const static wl_output_listener   S_OUTPUT_LISTENERS      = {output_geometry, output_mode, output_done, output_scale};

  auto *d = static_cast<display *>(_data);
  if (strcmp(_interface, wl_compositor_interface.name) == 0) {
    d->compositor_ = static_cast<wl_compositor *>(wl_registry_bind(_registry, _id, &wl_compositor_interface, 4));
  } else if (strcmp(_interface, xdg_wm_base_interface.name) == 0) {
    d->xdg_wm_base_ = static_cast<xdg_wm_base *>(wl_registry_bind(_registry, _id, &xdg_wm_base_interface, 1));
    xdg_wm_base_add_listener(d->xdg_wm_base_, &S_XDG_WM_BASE_LISTENERS, _data);
  } else if (strcmp(_interface, wl_seat_interface.name) == 0) {
    d->seat_ = static_cast<wl_seat *>(wl_registry_bind(_registry, _id, &wl_seat_interface, 5));
    wl_seat_add_listener(d->seat_, &S_SEAT_LISTENERS, _data);
  } else if (strcmp(_interface, wl_shm_interface.name) == 0) {
    d->shm_ = static_cast<wl_shm *>(wl_registry_bind(_registry, _id, &wl_shm_interface, 1));
  } else if (strcmp(_interface, wl_data_device_manager_interface.name) == 0) {
    d->data_device_manager_
        = static_cast<wl_data_device_manager *>(wl_registry_bind(_registry, _id, &wl_data_device_manager_interface, 3));
  } else if (strcmp(_interface, wl_output_interface.name) == 0) {
    d->output_ = static_cast<wl_output *>(wl_registry_bind(_registry, _id, &wl_output_interface, 2));
    wl_output_add_listener(d->output_, &S_OUTPUT_LISTENERS, _data);
  }
}

void registry_remove(void * /*unused*/, wl_registry * /*unused*/, u32 /*unused*/) {
  //std::cout << "registry_remove " << _id << std::endl;
}

void clipboard_sender::open(int _fd) {
  assert(fd_ == 0);
  assert(_fd != 0);
  fd_ = _fd;
}

ssize_t clipboard_sender::write(std::span<const u8> _data) const {
  assert(fd_ != 0);
  return ::write(fd_, _data.data(), _data.size_bytes());
}

void clipboard_sender::close() {
  if (fd_ != 0) {
    ::close(fd_);
    fd_ = 0;
  }
}

void clipboard_receiver::open(int _fd) {
  assert(fd_ == 0);
  assert(_fd != 0);
  fd_ = _fd;
}

ssize_t clipboard_receiver::read(std::span<u8> _data) const {
  assert(fd_ != 0);
  return ::read(fd_, _data.data(), _data.size_bytes());
}

void clipboard_receiver::close() {
  assert(fd_ != 0);
  ::close(fd_);
  fd_ = 0;
}

void display::data_offer_handle_offer(void *_data, wl_data_offer *_offer, const char *_mime) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "data_offer_handle_offer " << _offer << ", " << _mime << std::endl;
#endif
  auto *d      = static_cast<display *>(_data);
  auto  format = mime_type_format(_mime);
  if (format) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
    std::cout << "data_offer_handle_offer got " << *format << std::endl;
#endif
    d->offer_params_[_offer].formats_ |= *format;
  }
}

void display::data_offer_handle_source_actions(void *_data, wl_data_offer *_offer, u32 _actions) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "data_offer_handle_source_actions " << _offer << ", " << _actions << std::endl;
#endif
  auto *d = static_cast<display *>(_data);

  dragndrop_actions actions;
  if ((_actions & WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY) != 0u)
    actions.set(DNDCOPY);
  if ((_actions & WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE) != 0u)
    actions.set(DNDMOVE);

  d->offer_params_[_offer].actions_ = actions;
}

void display::data_offer_handle_action(void *_data, wl_data_offer *_offer, u32 _action) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "data_offer_handle_action " << _offer << ", " << _action << std::endl;
#endif
  auto            *d = static_cast<display *>(_data);
  dragndrop_action action;
  switch (_action) {
    case WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY: action = DNDCOPY; break;
    case WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE: action = DNDMOVE; break;
    default: action = DNDNONE; break;
  }
  d->offer_params_[_offer].drop_action_ = action;
}

void display::data_device_handle_data_offer(void *_data, wl_data_device * /*unused*/, wl_data_offer *_offer) {
  const static struct wl_data_offer_listener S_DATA_OFFER_LISTENERS
      = {data_offer_handle_offer, data_offer_handle_source_actions, data_offer_handle_action};

#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "data_device_handle_data_offer " << _offer << std::endl;
#endif
  auto *d = static_cast<display *>(_data);
  d->offer_params_[_offer].formats_.clear();
  d->offer_params_[_offer].actions_.clear();
  d->offer_params_[_offer].drop_format_ = FTEXT_PLAIN;
  d->offer_params_[_offer].drop_action_ = DNDNONE;
  wl_data_offer_add_listener(_offer, &S_DATA_OFFER_LISTENERS, _data);
}

void display::data_device_handle_enter(void *_data, wl_data_device *_device, u32 _serial, wl_surface *_surface,
                                       wl_fixed_t _x, wl_fixed_t _y, wl_data_offer *_offer) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "data_device_handle_enter " << _offer << ", " << _serial << ", " << wl_fixed_to_double(_x) << ", "
            << wl_fixed_to_double(_y) << std::endl;
#endif
  auto *d               = static_cast<display *>(_data);
  d->drag_enter_serial_ = _serial;

  if (d->last_offer_from_dropenter_ != nullptr) {
    wl_data_offer_finish(d->last_offer_from_dropenter_);
    wl_data_offer_destroy(d->last_offer_from_dropenter_);
    d->offer_params_.erase(d->last_offer_from_dropenter_);
  }
  d->last_offer_from_dropenter_ = _offer;

  auto wit = d->windows_.find(_surface);
  if (wit != d->windows_.end()) {
    const auto &itf = wit->second->drop_target_interface_;
    if (itf) {
      d->current_drop_target_ = wit->second;
      auto &params            = d->offer_params_[_offer];
      itf->on_enter(params.actions_, params.formats_);
    }
  }
}

void display::data_device_handle_leave(void *_data, wl_data_device *_device) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "data_device_handle_leave " << _device << std::endl;
#endif
  auto *d = static_cast<display *>(_data);
  if (d->current_drop_target_ != nullptr) {
    d->current_drop_target_ = nullptr;
  }
}

void display::data_device_handle_motion(void *_data, wl_data_device *_device, u32 _time, wl_fixed_t _x, wl_fixed_t _y) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  //std::cout << "data_device_handle_motion " << _device << ", " << _time << ", " << wl_fixed_to_double(_x) << ", " << wl_fixed_to_double(_y) << std::endl;
#endif
  auto *d = static_cast<display *>(_data);

  if (d->current_drop_target_ != nullptr) {
    auto &itf = d->current_drop_target_->drop_target_interface_;
    assert(itf);
    auto *offer  = d->last_offer_from_dropenter_;
    u32   scale  = d->current_drop_target_->scale_;
    auto &params = d->offer_params_[offer];
    vec2  pos{wl_fixed_to_double(_x), wl_fixed_to_double(_y)};

    auto move_result = itf->on_move(pos);
    switch (move_result.preferred_action_) {
      case DNDCOPY: d->cursor(CCOPY, scale); break;
      case DNDMOVE: d->cursor(CMOVE, scale); break;
      case DNDNONE: d->cursor(CNO_DROP, scale); break;
    }
    if (move_result.preferred_action_ == DNDNONE) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
      std::cout << "data_device_handle_motion canceling" << std::endl;
#endif
      wl_data_offer_accept(offer, d->drag_enter_serial_, nullptr);
    } else {
      u32                               possible  = 0;
      wl_data_device_manager_dnd_action preferred = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
      if (move_result.possible_actions_ & DNDCOPY) {
        possible |= WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
        preferred = WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
      } else if (move_result.possible_actions_ & DNDMOVE) {
        possible |= WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
        preferred = WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
      }
      wl_data_offer_set_actions(offer, possible, preferred);
      wl_data_offer_accept(offer, d->drag_enter_serial_, format_mime_type(move_result.preferred_format_));
      params.drop_format_ = move_result.preferred_format_;
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
      //std::cout << "data_device_handle_motion set " << possible << ", " << preferred << ", " << format_mime_type(move_result.preferred_format_) << std::endl;
#endif
    }
  }
}

void display::data_device_handle_drop(void *_data, wl_data_device *_device) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "data_device_handle_drop " << _device << std::endl;
#endif
  auto *d = static_cast<display *>(_data);

  if (d->current_drop_target_ != nullptr) {
    auto &itf = d->current_drop_target_->drop_target_interface_;
    assert(itf);
    auto *offer    = d->last_offer_from_dropenter_;
    auto  itparams = d->offer_params_.find(offer);
    assert(itparams != d->offer_params_.end());
    auto format = itparams->second.drop_format_;
    auto action = itparams->second.drop_action_;

    d->offer_params_.erase(offer);
    d->last_offer_from_dropenter_ = nullptr;

    int fds[2];
    pipe(fds);
    wl_data_offer_receive(offer, format_mime_type(format), fds[1]);
    close(fds[1]);

    auto reader = d->current_drop_readers_.emplace(d->current_drop_readers_.end());
    assert(d->current_drop_readers_.size() < 8);  // probable leak
    reader->receiver_.open(fds[0]);
    reader->thread_ = std::thread([d, offer, itf, action, reader]() {
      itf->on_drop(action, reader->receiver_);
      wl_data_offer_finish(offer);
      wl_data_offer_destroy(offer);
      d->post([d, reader](auto) { d->current_drop_readers_.erase(reader); });
    });
  }
}

void display::data_device_handle_selection(void *_data, wl_data_device *_device, wl_data_offer *_offer) {
#ifdef HUT_DEBUG_WL_DATA_LISTENERS
  std::cout << "data_device_handle_selection " << _device << ", " << _offer << std::endl;
#endif
  auto *d = static_cast<display *>(_data);
  if (d->last_offer_from_clipboard_ != nullptr) {
    wl_data_offer_finish(d->last_offer_from_clipboard_);
    wl_data_offer_destroy(d->last_offer_from_clipboard_);
    d->offer_params_.erase(d->last_offer_from_clipboard_);
  }
  d->last_offer_from_clipboard_ = _offer;
}

display::display(const char *_app_name, u32 _app_version, const char *_name)
    : keyboard_repeat_ctx_{*this}
    , animate_cursor_ctx_{*this} {
  HUT_PROFILE_FUN(PWAYLAND)
  std::vector<const char *> extensions = {VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
  init_vulkan_instance(_app_name, _app_version, extensions);

  display_ = wl_display_connect(_name != nullptr ? _name : getenv("HUT_DISPLAY"));
  if (display_ == nullptr)
    throw std::runtime_error("couldn't connect to wayland server");

  const static wl_registry_listener    S_REG_LISTENERS         = {registry_handler, registry_remove};
  const static wl_data_device_listener S_DATA_DEVICE_LISTENERS = {
      data_device_handle_data_offer, data_device_handle_enter, data_device_handle_leave,
      data_device_handle_motion,     data_device_handle_drop,  data_device_handle_selection,
  };

  xkb_context_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

  registry_ = wl_display_get_registry(display_);
  wl_registry_add_listener(registry_, &S_REG_LISTENERS, this);
  wl_display_roundtrip(display_);

  if (seat_ != nullptr) {
    keyboard_repeat_ctx_.thread_ = std::thread(keyboard_repeat_thread, &keyboard_repeat_ctx_);
  }

  if (data_device_manager_ != nullptr && seat_ != nullptr) {
    data_device_ = wl_data_device_manager_get_data_device(data_device_manager_, seat_);
    wl_data_device_add_listener(data_device_, &S_DATA_DEVICE_LISTENERS, this);
  }

  if (compositor_ != nullptr) {
    auto *dummy = wl_compositor_create_surface(compositor_);
    if (dummy == nullptr)
      throw std::runtime_error("couldn't create dummy surface");

    VkWaylandSurfaceCreateInfoKHR info = {};
    info.sType                         = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    info.display                       = display_;
    info.surface                       = dummy;

    auto        *func = get_proc<PFN_vkCreateWaylandSurfaceKHR>("vkCreateWaylandSurfaceKHR");
    VkSurfaceKHR dummy_surface;
    VkResult     vkr;
    HUT_VVK(func(instance_, &info, nullptr, &dummy_surface));

    init_vulkan_device(dummy_surface);

    HUT_PVK(vkDestroySurfaceKHR, instance_, dummy_surface, nullptr);
    wl_surface_destroy(dummy);
  } else {
    init_vulkan_device(nullptr);
  }

  if (shm_ != nullptr) {
    cursor_surface_             = wl_compositor_create_surface(compositor_);
    animate_cursor_ctx_.thread_ = std::thread(animate_cursor_thread, &animate_cursor_ctx_);
  }
}

display::~display() {
  HUT_PROFILE_FUN(PWAYLAND)
  preflush_jobs_.clear();
  postflush_garbage_.clear();
  posted_jobs_.clear();

  destroy_vulkan();

  if (animate_cursor_ctx_.thread_.joinable()) {
    {
      std::scoped_lock lock(animate_cursor_ctx_.mutex_);
      animate_cursor_ctx_.cursor_       = nullptr;
      animate_cursor_ctx_.stop_request_ = true;
    }
    animate_cursor_ctx_.cv_.notify_all();
    animate_cursor_ctx_.thread_.join();
  }
  if (keyboard_repeat_ctx_.thread_.joinable()) {
    {
      std::scoped_lock lock(keyboard_repeat_ctx_.mutex_);
      keyboard_repeat_ctx_.key_          = 0;
      keyboard_repeat_ctx_.stop_request_ = true;
    }
    keyboard_repeat_ctx_.cv_.notify_all();
    keyboard_repeat_ctx_.thread_.join();
  }

  if (keymap_ != nullptr)
    xkb_keymap_unref(keymap_);
  if (xkb_state_ != nullptr)
    xkb_state_unref(xkb_state_);
  if (xkb_state_empty_ != nullptr)
    xkb_state_unref(xkb_state_empty_);
  if (xkb_context_ != nullptr)
    xkb_context_unref(xkb_context_);

  if (last_offer_from_clipboard_ != nullptr)
    wl_data_offer_destroy(last_offer_from_clipboard_);
  if (last_offer_from_dropenter_ != nullptr)
    wl_data_offer_destroy(last_offer_from_dropenter_);
  if (data_device_ != nullptr)
    wl_data_device_destroy(data_device_);
  if (data_device_manager_ != nullptr)
    wl_data_device_manager_destroy(data_device_manager_);
  if (cursor_surface_ != nullptr)
    wl_surface_destroy(cursor_surface_);
  for (auto theme : cursor_themes_)
    wl_cursor_theme_destroy(theme.second);
  if (shm_ != nullptr)
    wl_shm_destroy(shm_);
  if (keyboard_ != nullptr)
    wl_keyboard_destroy(keyboard_);
  if (pointer_ != nullptr)
    wl_pointer_destroy(pointer_);
  if (seat_ != nullptr)
    wl_seat_destroy(seat_);
  if (xdg_wm_base_ != nullptr)
    xdg_wm_base_destroy(xdg_wm_base_);
  if (compositor_ != nullptr)
    wl_compositor_destroy(compositor_);
  if (output_ != nullptr)
    wl_output_destroy(output_);
  if (registry_ != nullptr)
    wl_registry_destroy(registry_);
  if (display_ != nullptr)
    wl_display_disconnect(display_);
}

void display::flush() {
  HUT_PROFILE_FUN(PWAYLAND)
  wl_display_flush(display_);
}

void display::post_empty_event() {
  HUT_PROFILE_FUN(PWAYLAND)
  wl_callback_destroy(wl_display_sync(display_));
}

void display::roundtrip() {
  HUT_PROFILE_FUN(PWAYLAND)
  wl_display_dispatch(display_);
}

int display::dispatch() {
  HUT_PROFILE_FUN(PWAYLAND)
  if (windows_.empty())
    throw std::runtime_error("dispatch called without any window");

  while (loop_) {
    HUT_PROFILE_SCOPE(PWAYLAND, "Main loop")
    roundtrip();
    process_posts(display::clock::now());
    if (windows_.empty())
      loop_ = false;
    for (auto wpair : windows_) {
      window *w = wpair.second;

      if (w->on_pause_.pending())
        HUT_PROFILE_FLUSH_BEVENT(w, on_pause_);
      if (w->on_resume_.pending())
        HUT_PROFILE_FLUSH_BEVENT(w, on_resume_);

      if (w->on_resize_.pending()) {
        w->size_ = std::get<0>(*w->on_resize_.args_);
        w->scale_ = std::get<1>(*w->on_resize_.args_);
        cursor(w->current_cursor_type_, w->scale_);
        wl_surface_set_buffer_scale(w->wayland_surface_, int(w->scale_));
        w->init_vulkan_surface();
        HUT_PROFILE_FLUSH_BEVENT(w, on_resize_);
      }

      if (w->invalidated_) {
        w->redraw(display::clock::now());
        w->invalidated_ = false;
      }
    }
#ifdef HUT_ENABLE_PROFILING
    profiling::threads_data::next_frame();
#endif  // HUT_ENABLE_PROFILING
  }

  return EXIT_SUCCESS;
}

void display::cursor_frame(wl_cursor *_cursor, size_t _frame) {
  if (_cursor != nullptr) {
    assert(_frame < _cursor->image_count);
    wl_cursor_image *image  = _cursor->images[_frame];
    wl_buffer       *buffer = wl_cursor_image_get_buffer(image);
    wl_surface_attach(cursor_surface_, buffer, 0, 0);
    wl_surface_damage(cursor_surface_, 0, 0, i32(image->width), i32(image->height));
    wl_surface_commit(cursor_surface_);
    wl_pointer_set_cursor(pointer_, last_mouse_enter_serial_, cursor_surface_, int(image->hotspot_x),
                          int(image->hotspot_y));
  } else {
    wl_pointer_set_cursor(pointer_, last_mouse_enter_serial_, nullptr, 0, 0);
  }
}

void display::cursor(cursor_type _c, u32 _scale) {
  if (pointer_ == nullptr)
    return;

  wl_cursor_theme *theme = nullptr;
  {
    std::scoped_lock sl(cursor_themes_mutex_);
    auto             themeit = cursor_themes_.find(_scale);
    if (themeit == cursor_themes_.end()) {
      theme                  = cursor_theme_load(_scale);
      cursor_themes_[_scale] = theme;
    } else {
      theme = themeit->second;
    }
  }
  assert(theme != nullptr);

  wl_cursor *result = wl_cursor_theme_get_cursor(theme, cursor_css_name(_c));
  if (result != nullptr) {
    if (result->image_count == 1) {
      animate_cursor(nullptr);
      cursor_frame(result, 0);
    } else {
      animate_cursor(result);
    }
  } else if (_c == CNONE) {
    animate_cursor(nullptr);
    cursor_frame(nullptr, 0);
  }
}

void display::animate_cursor(wl_cursor *_cursor) {
  if (_cursor != animate_cursor_ctx_.cursor_) {
    {
      std::scoped_lock lock(animate_cursor_ctx_.mutex_);
      animate_cursor_ctx_.cursor_ = _cursor;
      animate_cursor_ctx_.frame_  = 0;
    }
    animate_cursor_ctx_.cv_.notify_all();
  }
}

wl_cursor_theme *display::cursor_theme_load(u32 _scale) {
  HUT_PROFILE_FUN(PWAYLAND, _scale)
  char  *env_cursor_theme       = getenv("XCURSOR_THEME");
  char  *env_cursor_size        = getenv("XCURSOR_SIZE");
  size_t env_cursor_size_length = env_cursor_size == nullptr ? 0 : strlen(env_cursor_size);
  int    cursor_size            = 24;
  if (env_cursor_size_length > 0)
    std::from_chars(env_cursor_size, env_cursor_size + env_cursor_size_length, cursor_size);

  assert(shm_);
  return wl_cursor_theme_load(env_cursor_theme, cursor_size * i16(_scale), shm_);
}

void display::animate_cursor_thread(animate_cursor_context *_ctx) {
  std::unique_lock lock(_ctx->mutex_);
  while (!_ctx->stop_request_) {
    if (_ctx->cursor_ == nullptr)
      _ctx->cv_.wait(lock);
    if (_ctx->cursor_ != nullptr) {
      auto *cur_frame = _ctx->cursor_->images[_ctx->frame_];
      _ctx->display_.cursor_frame(_ctx->cursor_, _ctx->frame_++);
      if (_ctx->frame_ >= _ctx->cursor_->image_count)
        _ctx->frame_ = 0;
      _ctx->cv_.wait_for(lock, std::chrono::milliseconds(cur_frame->delay));
    }
  }
}

void display::keyboard_repeat(char32_t _c) {
  {
    std::scoped_lock lock(keyboard_repeat_ctx_.mutex_);
    keyboard_repeat_ctx_.key_   = _c;
    keyboard_repeat_ctx_.start_ = clock::now();
  }
  keyboard_repeat_ctx_.cv_.notify_all();
}

void display::keyboard_repeat_thread(keyboard_repeat_context *_ctx) {
  std::unique_lock lock(_ctx->mutex_);
  while (!_ctx->stop_request_) {
    if (_ctx->key_ == 0)
      _ctx->cv_.wait(lock);
    if (_ctx->key_ != 0u) {
      auto *w = _ctx->display_.keyboard_current_.second;
      if (w == nullptr) {
        _ctx->key_ = 0;
        continue;
      }
      auto now     = clock::now();
      auto elapsed = now - _ctx->start_;
      if (elapsed < _ctx->delay_) {
        auto initial_delay_remaining = _ctx->delay_ - elapsed;
        _ctx->cv_.wait_for(lock, initial_delay_remaining);
      } else {
        _ctx->display_.post([w, k = _ctx->key_](auto _tp) { HUT_PROFILE_EVENT(w, on_char_, (u32)k); });
        _ctx->cv_.wait_for(lock, _ctx->sleep_);
      }
    }
  }
}

}  // namespace hut
