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

#include "hut/renderdoc/renderdoc.hpp"

#include <filesystem>

#include <dlfcn.h>
#include <renderdoc_app.h>

namespace hut::renderdoc {

void configure(RENDERDOC_API_1_4_1 *_api) {
  auto curdir = std::filesystem::current_path();
  _api->SetCaptureFilePathTemplate("hut_renderdoc");
  _api->SetLogFilePathTemplate("hut_renderdoc");
  _api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, 1);
  _api->SetCaptureOptionU32(eRENDERDOC_Option_VerifyBufferAccess, 1);
  _api->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources, 1);
  _api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureAllCmdLists, 1);
  _api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 1);
}

RENDERDOC_API_1_4_1 *init() {
  RENDERDOC_API_1_4_1 *rdoc_api = nullptr;
  if (void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
    auto get_api = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
    if (get_api(eRENDERDOC_API_Version_1_4_1, (void **)&rdoc_api) == 1) {
      configure(rdoc_api);
    }
  }
  return rdoc_api;
}

RENDERDOC_API_1_4_1 *renderdoc_get() {
  static RENDERDOC_API_1_4_1 *s_rdoc_api = init();
  return s_rdoc_api;
}

void frame_begin() {
  auto *api = renderdoc_get();
  if (api == nullptr)
    return;

  api->StartFrameCapture(nullptr, nullptr);
}

void frame_end(const char *_comment) {
  auto *api = renderdoc_get();
  if (api == nullptr)
    return;

  api->EndFrameCapture(nullptr, nullptr);
  if (_comment != nullptr)
    api->SetCaptureFileComments(nullptr, _comment);
}

}  // namespace hut::renderdoc
