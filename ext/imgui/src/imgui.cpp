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

#include "hut/imgui/imgui.hpp"

#include <atomic>
#include <iostream>

#include "hut/utils/utf.hpp"

#include "hut/buffer.hpp"
#include "hut/display.hpp"
#include "hut/image.hpp"
#include "hut/pipeline.hpp"
#include "hut/window.hpp"

#include "imgui_refl.hpp"

using namespace hut;
using namespace imgui;

using imgui_pipeline = pipeline<ImDrawIdx, imgui_vert_spv_refl, imgui_frag_spv_refl, const shared_ubo &, const shared_image &, const shared_sampler &>;

constexpr size_t HUT_IMGUI_MAX_DESCRIPTORS = 256;

struct hut_imgui_mesh {
  imgui_pipeline::shared_indices  indices_;
  imgui_pipeline::shared_vertices vertices_;
};

struct tex_binding {
  shared_image   image_;
  shared_sampler sampler_;
};

struct hut_imgui_impl_ctx {
  display *display_ = nullptr;
  window  *window_  = nullptr;

  shared_buffer                   buffer_;
  shared_ubo                      ubo_;
  std::unique_ptr<imgui_pipeline> pipeline_;
  std::vector<hut_imgui_mesh>     meshes_;
  std::vector<tex_binding>        bindings_;
  std::vector<size_t>             reusable_bindings_ids_;

  display::time_point last_frame_;
  std::string         clipboard_buffer_;
  cursor_type         last_mouse_cursor_ = CDEFAULT;

  void reset() {
    meshes_.clear();
    pipeline_.reset();
    ubo_.reset();
    bindings_.clear();
    buffer_.reset();
    clipboard_buffer_.clear();
    last_mouse_cursor_ = CDEFAULT;
  }
};

static hut_imgui_impl_ctx g_ctx;

const char *GetClipboardText(void *) {
  auto &target = g_ctx.clipboard_buffer_;
  target.clear();

  std::atomic_bool done = false;
  g_ctx.window_->clipboard_receive(clipboard_formats{FTEXT_PLAIN},
                                   [&](clipboard_format _mime, clipboard_receiver &_receiver) {
                                     u8     buffer[2048];
                                     size_t read_bytes;
                                     while ((read_bytes = _receiver.read(buffer))) {
                                       target += std::string_view{(char *)buffer, read_bytes};
                                     }
                                     done = true;
                                   });

  // As this function is blocking the main thread, due to the nature of the synchronous clipboard API,
  // we need to continue to process wayland events to continue processing the clipboard communication
  while (!done)
    g_ctx.display_->roundtrip();

  return target.data();
}

void SetClipboardText(void *, const char *text) {
  g_ctx.window_->clipboard_offer(clipboard_formats{FTEXT_PLAIN},
                                 [buffer{std::string(text)}](clipboard_format _mime, clipboard_sender &_sender) {
                                   if (_mime == FTEXT_PLAIN) {
                                     _sender.write({(u8 *)buffer.data(), buffer.size()});
                                   }
                                 });
}

bool ImGui_ImplHut_Init(display *_d, window *_w, bool _install_callbacks) {
  ImGuiIO &io            = ImGui::GetIO();
  io.BackendPlatformName = io.BackendRendererName = "hut_imgui";
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

  io.GetClipboardTextFn = GetClipboardText;
  io.SetClipboardTextFn = SetClipboardText;

  g_ctx.display_    = _d;
  g_ctx.window_     = _w;
  g_ctx.last_frame_ = display::clock::now();

  if (_install_callbacks) {
    _w->on_resize.connect(ImGui_ImplHut_HandleOnResize);
    _w->on_mouse.connect(ImGui_ImplHut_HandleOnMouse);
    _w->on_key.connect(ImGui_ImplHut_HandleOnKey);
    _w->on_kmods.connect(ImGui_ImplHut_HandleOnKMods);
    _w->on_char.connect(ImGui_ImplHut_HandleOnChar);
    _w->on_blur.connect(ImGui_ImplHut_HandleOnBlur);
    _w->on_focus.connect(ImGui_ImplHut_HandleOnFocus);
  }

  ImGui_ImplHut_HandleOnResize(_w->size());

  return true;
}

void ImGui_ImplHut_Shutdown() {
  ImGui_ImplHut_InvalidateDeviceObjects();
}

void ImGui_ImplHut_NewFrame() {
  if (!g_ctx.buffer_)
    ImGui_ImplHut_CreateDeviceObjects();

  const auto now    = display::clock::now();
  const auto delta  = std::chrono::duration<float>(now - g_ctx.last_frame_);
  g_ctx.last_frame_ = now;

  ImGuiIO &io  = ImGui::GetIO();
  io.DeltaTime = std::max(delta.count(), std::numeric_limits<float>::min());
}

void ImGui_ImplHut_RenderDrawData(VkCommandBuffer _cmd_buffer, ImDrawData *_draw_data) {
  if (_draw_data->DisplaySize.x <= 0.0f || _draw_data->DisplaySize.y <= 0.0f)
    return;

  static_assert(std::is_same_v<imgui_pipeline::index_t, ImDrawIdx>);
  // check bitcast compatibility between our pipeline vertex (generated from reflection) and ImDrawVert
  static_assert(sizeof(imgui_pipeline::vertex) == sizeof(ImDrawVert));
  static_assert(offsetof(imgui_pipeline::vertex, pos_) == offsetof(ImDrawVert, pos));
  static_assert(offsetof(imgui_pipeline::vertex, uv_) == offsetof(ImDrawVert, uv));
  static_assert(offsetof(imgui_pipeline::vertex, col_) == offsetof(ImDrawVert, col));

  g_ctx.meshes_.clear();
  for (int n = 0; n < _draw_data->CmdListsCount; n++) {
    const ImDrawList *cmd_list = _draw_data->CmdLists[n];

    hut_imgui_mesh m;

    m.indices_         = g_ctx.buffer_->allocate<imgui_pipeline::index_t>(cmd_list->IdxBuffer.Size);
    auto imgui_indices = std::span{(imgui_pipeline::index_t *)cmd_list->IdxBuffer.Data, uint(cmd_list->IdxBuffer.Size)};
    m.indices_->set(imgui_indices);

    m.vertices_         = g_ctx.buffer_->allocate<imgui_pipeline::vertex>(cmd_list->VtxBuffer.Size);
    auto imgui_vertices = std::span{(imgui_pipeline::vertex *)cmd_list->VtxBuffer.Data, uint(cmd_list->VtxBuffer.Size)};
    m.vertices_->set(imgui_vertices);

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];

      if (pcmd->UserCallback != nullptr && pcmd->UserCallback != ImDrawCallback_ResetRenderState) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        VkRect2D scissor = {};
        scissor.offset   = {(i32)pcmd->ClipRect.x, (i32)pcmd->ClipRect.y};
        scissor.extent   = {(u32)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                          (u32)(pcmd->ClipRect.w - pcmd->ClipRect.y)};
        HUT_PVK(vkCmdSetScissor, _cmd_buffer, 0, 1, &scissor);

        g_ctx.pipeline_->draw(_cmd_buffer,
                              (size_t)pcmd->TextureId,
                              m.indices_, pcmd->IdxOffset, pcmd->ElemCount,
                              imgui_pipeline::shared_instances{}, 0, 1,
                              m.vertices_, pcmd->VtxOffset);
      }
    }

    g_ctx.meshes_.emplace_back(std::move(m));
  }

  // Reset scissor
  ImGuiIO &io      = ImGui::GetIO();
  VkRect2D scissor = {};
  scissor.offset   = {0, 0};
  scissor.extent   = {static_cast<u32>(io.DisplaySize[0]), static_cast<u32>(io.DisplaySize[1])};
  HUT_PVK(vkCmdSetScissor, _cmd_buffer, 0, 1, &scissor);
}

bool ImGui_ImplHut_CreateDeviceObjects() {
  // Build texture atlas
  ImGuiIO       &io = ImGui::GetIO();
  unsigned char *data;
  int            width, height;
  io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

  g_ctx.buffer_ = std::make_shared<buffer>(*g_ctx.display_, 4 * 1024 * 1024);

  g_ctx.ubo_ = g_ctx.buffer_->allocate<common_ubo>(1, g_ctx.display_->ubo_align());
  g_ctx.ubo_->set(common_ubo{g_ctx.window_->size()});

  // Upload texture to graphics system
  std::span<u8> pixels{data, size_t(width * height * 4)};
  image_params  atlas_params;
  atlas_params.size_   = {width, height};
  atlas_params.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  auto image           = image::load_raw(*g_ctx.display_, pixels, width * 4, atlas_params);
  auto samp            = std::make_shared<sampler>(*g_ctx.display_);
  io.Fonts->TexID      = ImGui_ImplHut_AddImage(image, samp);

  pipeline_params pparams;
  pparams.max_sets_ = HUT_IMGUI_MAX_DESCRIPTORS;

  g_ctx.pipeline_ = std::make_unique<imgui_pipeline>(*g_ctx.window_, pparams);
  g_ctx.pipeline_->resize_descriptors(g_ctx.bindings_.size());
  for (uint i = 0; i < g_ctx.bindings_.size(); i++) {
    auto &binding = g_ctx.bindings_[i];
    g_ctx.pipeline_->write(i, g_ctx.ubo_, binding.image_, binding.sampler_);
  }

  g_ctx.window_->invalidate(true);
  return true;
}

void ImGui_ImplHut_InvalidateDeviceObjects() {
  g_ctx.reset();
}

ImTextureID ImGui_ImplHut_AddImage(const shared_image &_image, const shared_sampler &_sampler) {
  size_t index;
  if (!g_ctx.reusable_bindings_ids_.empty()) {
    index = g_ctx.reusable_bindings_ids_.back();
    g_ctx.reusable_bindings_ids_.pop_back();
  } else {
    index = g_ctx.bindings_.size();
    assert(index < HUT_IMGUI_MAX_DESCRIPTORS);
    g_ctx.bindings_.emplace_back(tex_binding{_image, _sampler});
  }

  if (g_ctx.pipeline_)
    g_ctx.pipeline_->write(index, g_ctx.ubo_, _image, _sampler);

  return reinterpret_cast<ImTextureID>(index);
}

void ImGui_ImplHut_RemImage(ImTextureID _id) {
  g_ctx.reusable_bindings_ids_.emplace_back(reinterpret_cast<size_t>(_id));
}

bool ImGui_ImplHut_HandleOnResize(uvec2 _size) {
  ImGuiIO &io    = ImGui::GetIO();
  io.DisplaySize = {float(_size.x), float(_size.y)};

  if (g_ctx.ubo_)
    g_ctx.ubo_->set(common_ubo{_size});

  return false;
}

cursor_type hut_cursor_type(ImGuiMouseCursor _input) {
  cursor_type result;
  switch (_input) {
    case ImGuiMouseCursor_Arrow: result = CDEFAULT; break;
    case ImGuiMouseCursor_TextInput: result = CTEXT; break;
    case ImGuiMouseCursor_ResizeAll: result = CMOVE; break;
    case ImGuiMouseCursor_ResizeNS: result = CRESIZE_NS; break;
    case ImGuiMouseCursor_ResizeEW: result = CRESIZE_EW; break;
    case ImGuiMouseCursor_ResizeNESW: result = CRESIZE_NESW; break;
    case ImGuiMouseCursor_ResizeNWSE: result = CRESIZE_NWSE; break;
    case ImGuiMouseCursor_Hand: result = CPOINTER; break;
    case ImGuiMouseCursor_NotAllowed: result = CNOT_ALLOWED; break;
    default:
      assert(false);
  }
  return result;
}

bool ImGui_ImplHut_HandleOnMouse(u8 _button, mouse_event_type _type, vec2 _pos) {
  ImGuiIO &io = ImGui::GetIO();
  io.MousePos = {_pos.x, _pos.y};

  switch (_type) {
    case MDOWN:
      assert(_button > 0 && _button < ImGuiMouseButton_COUNT);
      io.MouseDown[_button - 1] = true;
      break;
    case MUP:
      assert(_button > 0 && _button < ImGuiMouseButton_COUNT);
      io.MouseDown[_button - 1] = false;
      break;
    case MWHEEL_DOWN:
      io.MouseWheel -= 1;
      break;
    case MWHEEL_UP:
      io.MouseWheel += 1;
      break;
    case MMOVE: /*do nothing*/ break;
  }

  auto request_cursor = io.MouseDrawCursor ? CNONE : hut_cursor_type(ImGui::GetMouseCursor());
  if (g_ctx.last_mouse_cursor_ != request_cursor) {
    g_ctx.last_mouse_cursor_ = request_cursor;
    g_ctx.window_->cursor(request_cursor);
  }

  return ImGui::GetIO().WantCaptureMouse;
}

ImGuiKey imgui_keysym(hut::keysym _ksym) {
  ImGuiKey ImGuiKey_Linefeed = ImGuiKey_None;
  ImGuiKey ImGuiKey_SysReq = ImGuiKey_None;

  switch (_ksym) {
#define HUT_MAP_KEYSYM(FORMAT_LINUX, FORMAT_X11, FORMAT_IMGUI) case KSYM_##FORMAT_LINUX: return ImGuiKey_##FORMAT_IMGUI;
#include "hut/keysyms.inc"
#undef HUT_MAP_KEYSYM
  }

  return ImGuiKey_None;
}

bool ImGui_ImplHut_HandleOnKey(keycode _kcode, keysym _ksym, bool _down) {
  ImGuiIO &io = ImGui::GetIO();

#if !defined(NDEBUG) && 0
  char  kcode_name[64];
  char *kcode_name_end = g_ctx.display_->keycode_name(kcode_name, _kcode);
  *kcode_name_end      = 0;

  char32_t kcode_idle_char = g_ctx.display_->keycode_idle_char(_kcode);
  char8_t  kcode_idle_char_utf8[5];
  char8_t *kcode_idle_char_utf8_end = to_utf8(kcode_idle_char_utf8, kcode_idle_char);
  *kcode_idle_char_utf8_end         = 0;

  std::cout << "OnKey "
            << "keycode:" << _kcode << " (name: " << kcode_name << ", char:" << (char *)kcode_idle_char_utf8 << "), "
            << "keysym: " << _ksym << " (" << keysym_name(_ksym) << "), "
            << "down:" << _down
            << std::endl;
#endif

  io.AddKeyEvent(imgui_keysym(_ksym), _down);

  return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGui_ImplHut_HandleOnKMods(modifiers _mods) {
  ImGuiIO &io = ImGui::GetIO();

#if !defined(NDEBUG) && 0
  std::cout << "OnMods " << _mods
            << "(shift: " << _mods.query(KMOD_SHIFT) << ", "
            << "ctrl: " << _mods.query(KMOD_CTRL) << ", "
            << "alt: " << _mods.query(KMOD_ALT) << ")"
            << std::endl;
#endif

  ImGuiKeyModFlags flags = 0;
  flags |= _mods.query(KMOD_SHIFT) ? ImGuiKeyModFlags_Shift : 0;
  flags |= _mods.query(KMOD_CTRL) ? ImGuiKeyModFlags_Ctrl : 0;
  flags |= _mods.query(KMOD_ALT) ? ImGuiKeyModFlags_Alt : 0;
  io.AddKeyModsEvent(flags);

  return false;
}

bool ImGui_ImplHut_HandleOnChar(char32_t _utf32) {
  ImGuiIO &io = ImGui::GetIO();

#if !defined(NDEBUG) && 0
  char8_t utf8[6];
  *to_utf8(utf8, _utf32) = 0;
  std::cout << "OnChar " << (char*)utf8 << " (0x" << std::hex << u32(_utf32) << std::dec << "), " << std::endl;
#endif

  io.AddInputCharacter(_utf32);
  return true;
}

bool ImGui_ImplHut_HandleOnBlur() {
  ImGuiIO &io = ImGui::GetIO();

#if !defined(NDEBUG) && 0
  std::cout << "OnBlur" << std::endl;
#endif

  io.AddFocusEvent(false);
  return true;
}

bool ImGui_ImplHut_HandleOnFocus() {
  ImGuiIO &io = ImGui::GetIO();

#if !defined(NDEBUG) && 0
  std::cout << "OnFocus" << std::endl;
#endif

  io.AddFocusEvent(true);
  return true;
}
