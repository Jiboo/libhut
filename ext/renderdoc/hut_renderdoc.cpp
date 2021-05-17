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

#include "hut_renderdoc.hpp"

#include <cassert>

#include <filesystem>

#include "renderdoc_app.h"

namespace hut::renderdoc {

  RENDERDOC_API_1_4_1 *renderdoc_get() {
    static RENDERDOC_API_1_4_1 *rdoc_api = details::init();
    return rdoc_api;
  }

  void frame_begin() {
    auto *api = renderdoc_get();
    if (!api) return;

    api->StartFrameCapture(nullptr, nullptr);
  }

  void frame_end(const char *_comment) {
    auto *api = renderdoc_get();
    if (!api) return;

    api->EndFrameCapture(nullptr, nullptr);
    if (_comment != nullptr)
      api->SetCaptureFileComments(nullptr, _comment);
  }

  void details::configure(RENDERDOC_API_1_4_1 *_api) {
    auto curdir = std::filesystem::current_path();
    _api->SetCaptureFilePathTemplate("hut_renderdoc");
    _api->SetLogFilePathTemplate("hut_renderdoc");
    _api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, 1);
    _api->SetCaptureOptionU32(eRENDERDOC_Option_VerifyBufferAccess, 1);
    _api->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources, 1);
    _api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureAllCmdLists, 1);
    _api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 1);
  }

} // ns hut::renderdoc
