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

#include "hut/pipeline.hpp"

#include "tst_shaders_refl.hpp"

namespace hut {

struct proj_ubo {
  mat4 proj_{1};

  explicit proj_ubo(u16vec2 _size) {
    proj_ = ortho<float>(0.f, float(_size.x), 0.f, float(_size.y));
  }
};
using shared_proj_ubo = buffer::shared_suballoc<proj_ubo>;

struct vp_ubo {
  mat4 proj_{1};
  mat4 view_{1};
};
using shared_vp_ubo = buffer::shared_suballoc<vp_ubo>;

using pipeline_atlas = pipeline<u16, tst_shaders::tex_vert_spv_refl, tst_shaders::atlas_frag_spv_refl,
                                const shared_proj_ubo &, const shared_atlas &, const shared_sampler &>;

using pipeleine_atlas_mask = pipeline<u16, tst_shaders::tex_mask_vert_spv_refl, tst_shaders::atlas_mask_frag_spv_refl,
                                      const shared_proj_ubo &, const shared_atlas &, const shared_sampler &>;

using pipeleine_box_rgba = pipeline<u16, tst_shaders::box_rgba_vert_spv_refl, tst_shaders::box_rgba_frag_spv_refl,
                                    const shared_proj_ubo &>;

using pipeleine_rgb = pipeline<u16, tst_shaders::rgb_vert_spv_refl, tst_shaders::rgb_frag_spv_refl,
                               const shared_proj_ubo &>;

using pipeleine_rgb3d = pipeline<u16, tst_shaders::rgb3d_vert_spv_refl, tst_shaders::rgb_frag_spv_refl,
                                 const shared_vp_ubo &>;

using pipeleine_rgba = pipeline<u16, tst_shaders::rgba_vert_spv_refl, tst_shaders::rgba_frag_spv_refl,
                                const shared_proj_ubo &>;

using pipeleine_skybox = pipeline<u16, tst_shaders::skybox_vert_spv_refl, tst_shaders::skybox_frag_spv_refl,
                                  const shared_vp_ubo &, const shared_image &, const shared_sampler &>;

using pipeleine_tex = pipeline<u16, tst_shaders::tex_vert_spv_refl, tst_shaders::tex_frag_spv_refl,
                               const shared_proj_ubo &, const shared_image &, const shared_sampler &>;

using pipeleine_tex_rgb = pipeline<u16, tst_shaders::tex_rgb_vert_spv_refl, tst_shaders::tex_rgb_frag_spv_refl,
                                   const shared_proj_ubo &, const shared_image &, const shared_sampler &>;

using pipeleine_tex_rgba = pipeline<u16, tst_shaders::tex_rgba_vert_spv_refl, tst_shaders::tex_rgba_frag_spv_refl,
                                    const shared_proj_ubo &, const shared_image &, const shared_sampler &>;

using pipeleine_tex_mask = pipeline<u16, tst_shaders::tex_mask_vert_spv_refl, tst_shaders::tex_mask_frag_spv_refl,
                                    const shared_proj_ubo &, const shared_image &, const shared_sampler &>;

}  // namespace hut
