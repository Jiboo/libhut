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

#include <iostream>

#include "imgui_impl_hut.h"

#include "hut/buffer_pool.hpp"
#include "hut/display.hpp"
#include "hut/image.hpp"
#include "hut/pipeline.hpp"
#include "hut/window.hpp"

#include "hut_imgui_shaders_refl.hpp"

// NOTE JBL: Must override the generated reflection, as we currently have no way for custom pragma/decoration/attribute to generate the color as R8G8B8A8 instead of R32G32B32A32, which is chosen because the type in the glsl is vec4
struct imgui_vert_spv_refl : hut::hut_imgui_shaders::imgui_vert_spv_refl {
  using instance = typename hut::hut_imgui_shaders::imgui_vert_spv_refl::instance;
  using vertex = ImDrawVert;

  constexpr static std::array<VkVertexInputAttributeDescription, 3> vertices_description_ {
    VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, pos)},
    VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex, uv)},
    VkVertexInputAttributeDescription{.location = 2, .binding = 0, .format = VK_FORMAT_R8G8B8A8_UNORM, .offset = offsetof(vertex, col)},
  };
};

using imgui_pipeline = hut::pipeline<hut::proj_ubo, ImDrawIdx, imgui_vert_spv_refl, hut::hut_imgui_shaders::imgui_frag_spv_refl, const hut::shared_image &, const hut::shared_sampler &>;

struct hut_imgui_mesh {
  imgui_pipeline::shared_indices indices_;
  imgui_pipeline::shared_vertices vertices_;
};

struct hut_imgui_impl_ctx {
  hut::display *display_ = nullptr;
  hut::window *window_ = nullptr;

  hut::shared_buffer buffer_;
  hut::shared_sampler sampler_;
  std::unique_ptr<imgui_pipeline> pipeline_;
  hut::shared_ref<hut::proj_ubo> ubo_;

  hut::display::time_point last_frame_;
  std::vector<hut::shared_image> images_;
  std::vector<hut_imgui_mesh> meshes_;

  std::string clipboard_buffer_;

  void reset() {
    meshes_.clear();
    sampler_.reset();
    pipeline_.reset();
    ubo_.reset();
    images_.clear();
    buffer_.reset();
  }
};

static hut_imgui_impl_ctx g_ctx;

const char* GetClipboardText(void* user_data) {
  // FIXME JBL: Will be broken once clipboard_receive is fully async
  auto &target = g_ctx.clipboard_buffer_;
  target.clear();
  g_ctx.window_->clipboard_receive(hut::FTEXT_PLAIN,
   [&](hut::clipboard_format _mime, hut::clipboard_receiver &_receiver) {
     uint8_t buffer[2048];
     size_t read_bytes;
     while ((read_bytes = _receiver.read(buffer))) {
       target += std::string_view{(char *) buffer, read_bytes};
#if !defined(NDEBUG) && 0
       std::cout << "GetClipboardText received " << read_bytes << " bytes" << std::endl;
#endif
     }
   });

  return target.data();
}

void SetClipboardText(void* user_data, const char* text) {
  g_ctx.window_->clipboard_offer(hut::FTEXT_PLAIN,
    [buffer{std::string(text)}](hut::clipboard_format _mime, hut::clipboard_sender &_sender) {
    if (_mime == hut::FTEXT_PLAIN) {
      auto wrote_bytes = _sender.write({(uint8_t *) buffer.data(), buffer.size()});
#if !defined(NDEBUG) && 0
      std::cout << "SetClipboardText sent " << wrote_bytes << " bytes" << std::endl;
#endif
    }
  });
}

bool ImGui_ImplHut_Init(hut::display *_d, hut::window* _w, bool _install_callbacks) {
  ImGuiIO& io = ImGui::GetIO();
  io.BackendPlatformName = io.BackendRendererName = "imgui_impl_hut";
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

  io.KeyMap[ImGuiKey_Tab] = hut::KSYM_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = hut::KSYM_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = hut::KSYM_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = hut::KSYM_UP;
  io.KeyMap[ImGuiKey_DownArrow] = hut::KSYM_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = hut::KSYM_PAGEUP;
  io.KeyMap[ImGuiKey_PageDown] = hut::KSYM_PAGEDOWN;
  io.KeyMap[ImGuiKey_Home] = hut::KSYM_HOME;
  io.KeyMap[ImGuiKey_End] = hut::KSYM_END;
  io.KeyMap[ImGuiKey_Insert] = hut::KSYM_INSERT;
  io.KeyMap[ImGuiKey_Delete] = hut::KSYM_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = hut::KSYM_BACKSPACE;
  io.KeyMap[ImGuiKey_Space] = hut::KSYM_SPACE;
  io.KeyMap[ImGuiKey_Enter] = hut::KSYM_ENTER;
  io.KeyMap[ImGuiKey_Escape] = hut::KSYM_ESC;
  io.KeyMap[ImGuiKey_KeyPadEnter] = hut::KSYM_KPENTER;
  io.KeyMap[ImGuiKey_A] = hut::KSYM_A;
  io.KeyMap[ImGuiKey_C] = hut::KSYM_C;
  io.KeyMap[ImGuiKey_V] = hut::KSYM_V;
  io.KeyMap[ImGuiKey_X] = hut::KSYM_X;
  io.KeyMap[ImGuiKey_Y] = hut::KSYM_Y;
  io.KeyMap[ImGuiKey_Z] = hut::KSYM_Z;

  io.GetClipboardTextFn = GetClipboardText;
  io.SetClipboardTextFn = SetClipboardText;

  g_ctx.display_ = _d;
  g_ctx.window_ = _w;
  g_ctx.last_frame_ = hut::display::clock::now();

  if (_install_callbacks) {
    _w->on_resize.connect(ImGui_ImplHut_HandleOnResize);
    _w->on_mouse.connect(ImGui_ImplHut_HandleOnMouse);
    _w->on_key.connect(ImGui_ImplHut_HandleOnKey);
    _w->on_kmods.connect(ImGui_ImplHut_HandleOnKMods);
    _w->on_char.connect(ImGui_ImplHut_HandleOnChar);
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

  const auto now = hut::display::clock::now();
  const auto delta = std::chrono::duration<float>(now - g_ctx.last_frame_);
  g_ctx.last_frame_ = now;

  ImGuiIO& io = ImGui::GetIO();
  io.DeltaTime = std::max(delta.count(), std::numeric_limits<float>::min());
}

void ImGui_ImplHut_RenderDrawData(VkCommandBuffer _cmd_buffer, ImDrawData* _draw_data) {
  if (_draw_data->DisplaySize.x <= 0.0f || _draw_data->DisplaySize.y <= 0.0f)
    return;

  static_assert(std::is_same_v<imgui_pipeline::indice, ImDrawIdx>);
  static_assert(std::is_same_v<imgui_pipeline::vertex, ImDrawVert>);

  g_ctx.meshes_.clear();
  for (int n = 0; n < _draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list = _draw_data->CmdLists[n];

    hut_imgui_mesh m;

    m.indices_ = g_ctx.buffer_->allocate<imgui_pipeline::indice>(cmd_list->IdxBuffer.Size);
    m.indices_->update_raw(0, m.indices_->size_bytes(), cmd_list->IdxBuffer.Data);

    m.vertices_ = g_ctx.buffer_->allocate<imgui_pipeline::vertex>(cmd_list->VtxBuffer.Size);
    m.vertices_->update_raw(0, m.vertices_->size_bytes(), cmd_list->VtxBuffer.Data);

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

      if (pcmd->UserCallback != NULL && pcmd->UserCallback != ImDrawCallback_ResetRenderState) {
        pcmd->UserCallback(cmd_list, pcmd);
      }
      else {
        VkRect2D scissor = {};
        scissor.offset = {(int32_t) pcmd->ClipRect.x, (int32_t) pcmd->ClipRect.y};
        scissor.extent = {(uint32_t) (pcmd->ClipRect.z - pcmd->ClipRect.x),
                          (uint32_t) (pcmd->ClipRect.w - pcmd->ClipRect.y)};
        HUT_PVK(vkCmdSetScissor, _cmd_buffer, 0, 1, &scissor);

        g_ctx.pipeline_->draw(_cmd_buffer,
                              (size_t) pcmd->TextureId,
                              m.indices_, pcmd->IdxOffset, pcmd->ElemCount,
                              imgui_pipeline::shared_instances{}, 0, 1,
                              m.vertices_, pcmd->VtxOffset);
      }
    }

    g_ctx.meshes_.emplace_back(std::move(m));
  }
}

bool ImGui_ImplHut_CreateDeviceObjects() {
  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  unsigned char *data;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

  g_ctx.buffer_ = g_ctx.display_->alloc_buffer(4*1024*1024);
  g_ctx.sampler_ = std::make_shared<hut::sampler>(*g_ctx.display_);

  hut::proj_ubo default_ubo;
  auto w_size = g_ctx.window_->size();
  default_ubo.proj_ = glm::ortho<float>(0, w_size.x, 0, w_size.y);
  g_ctx.ubo_ = g_ctx.display_->alloc_ubo(g_ctx.buffer_, default_ubo);

  g_ctx.images_.resize(1);

  // Upload texture to graphics system
  std::span<uint8_t> pixels {data, size_t(width * height * 4)};
  hut::image_params atlas_params;
  atlas_params.size_ = {width, height};
  atlas_params.format_ = VK_FORMAT_R8G8B8A8_UNORM;
  g_ctx.images_[0] = hut::image::load_raw(*g_ctx.display_, pixels, atlas_params);
  io.Fonts->TexID = (ImTextureID)0;

  g_ctx.pipeline_ = std::make_unique<imgui_pipeline>(*g_ctx.window_);
  g_ctx.pipeline_->write(0, g_ctx.ubo_, g_ctx.images_[0], g_ctx.sampler_);

  g_ctx.window_->invalidate(true);
  return true;
}

void ImGui_ImplHut_InvalidateDeviceObjects() {
  g_ctx.reset();
}

bool ImGui_ImplHut_HandleOnResize(hut::uvec2 _size) {
  ImGuiIO& io = ImGui::GetIO();
  const auto win_size = g_ctx.window_->size();
  io.DisplaySize = {(float)win_size.x, (float)win_size.y};
  return false;
}

hut::cursor_type hut_cursor_type(ImGuiMouseCursor _input) {
  hut::cursor_type result = hut::CDEFAULT;
  switch (_input) {
    case ImGuiMouseCursor_Arrow:        result = hut::CDEFAULT; break;
    case ImGuiMouseCursor_TextInput:    result = hut::CTEXT; break;
    case ImGuiMouseCursor_ResizeAll:    result = hut::CMOVE; break;
    case ImGuiMouseCursor_ResizeNS:     result = hut::CRESIZE_NS; break;
    case ImGuiMouseCursor_ResizeEW:     result = hut::CRESIZE_EW; break;
    case ImGuiMouseCursor_ResizeNESW:   result = hut::CRESIZE_NESW; break;
    case ImGuiMouseCursor_ResizeNWSE:   result = hut::CRESIZE_NWSE; break;
    case ImGuiMouseCursor_Hand:         result = hut::CPOINTER; break;
    case ImGuiMouseCursor_NotAllowed:   result = hut::CNOT_ALLOWED; break;
    default:
      assert(false); break;
  }
  return result;
}

bool ImGui_ImplHut_HandleOnMouse(uint8_t _button, hut::mouse_event_type _type, glm::vec2 _pos) {
  ImGuiIO& io = ImGui::GetIO();
  io.MousePos = {_pos.x, _pos.y};

  switch (_type) {
    case hut::MDOWN:
      assert(_button > 0 && _button < 6);
      io.MouseDown[_button - 1] = true; break;
    case hut::MUP:
      assert(_button > 0 && _button < 6);
      io.MouseDown[_button - 1] = false; break;
    case hut::MWHEEL_DOWN:
      io.MouseWheel -= 1; break;
    case hut::MWHEEL_UP:
      io.MouseWheel += 1; break;
    case hut::MMOVE: /*do nothing*/ break;
  }

  if (io.MouseDrawCursor) {
    g_ctx.window_->cursor(hut::CNONE);
  }
  else {
    g_ctx.window_->cursor(hut_cursor_type(ImGui::GetMouseCursor()));
  }

  return false;
}

bool ImGui_ImplHut_HandleOnKey(hut::keycode _kcode, hut::keysym _ksym, bool _down) {
  ImGuiIO& io = ImGui::GetIO();

#if !defined(NDEBUG) && 0
    char kcode_name[64];
    char *kcode_name_end = g_ctx.display_->keycode_name(kcode_name, _kcode);
    *kcode_name_end = 0;

    char32_t kcode_idle_char = g_ctx.display_->keycode_idle_char(_kcode);
    char8_t kcode_idle_char_utf8[5];
    char8_t *kcode_idle_char_utf8_end = hut::to_utf8(kcode_idle_char_utf8, kcode_idle_char);
    *kcode_idle_char_utf8_end = 0;

    std::cout << "OnKey "
              << "keycode:" << _kcode << " (name: " << kcode_name << ", char:" << (char*)kcode_idle_char_utf8 << "), "
              << "keysym: " << _ksym << " (" << hut::keysym_name(_ksym) << "), "
              << "down:" << _down
              << std::endl;
#endif

  static_assert(hut::KSYM_LAST_VALUE <= IM_ARRAYSIZE(io.KeysDown));
  io.KeysDown[_ksym] = _down;

  return false;
}

bool ImGui_ImplHut_HandleOnKMods(hut::modifiers _mods) {
  ImGuiIO& io = ImGui::GetIO();

#if !defined(NDEBUG) && 0
  std::cout << "OnMods " << _mods
            << "(shift: " << _mods.query(hut::KMOD_SHIFT) << ", "
            << "ctrl: " << _mods.query(hut::KMOD_CTRL) << ", "
            << "alt: " << _mods.query(hut::KMOD_ALT) << ")"
            << std::endl;
#endif

  io.KeyShift = _mods.query(hut::KMOD_SHIFT);
  io.KeyCtrl = _mods.query(hut::KMOD_CTRL);
  io.KeyAlt = _mods.query(hut::KMOD_ALT);
  io.KeySuper = false;

  return false;
}

bool ImGui_ImplHut_HandleOnChar(char32_t _utf32_char) {
  ImGuiIO& io = ImGui::GetIO();

  char8_t buffer[5];
  char8_t *end = hut::to_utf8(buffer, _utf32_char);
  *end = 0;

#if !defined(NDEBUG) && 0
  std::cout << "OnChar " << (char*)buffer << " (0x" << std::hex << (uint32_t)_utf32_char << std::dec << "), " << std::endl;
#endif

  io.AddInputCharactersUTF8((const char*)buffer);
  return false;
}
