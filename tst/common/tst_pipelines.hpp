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

struct proj_ubo {
  hut::mat4 proj_ {1};
};

struct vp_ubo {
  hut::mat4 proj_;
  hut::mat4 view_;
};

using atlas = hut::pipeline<uint16_t, hut::tst_shaders::tex_vert_spv_refl, hut::tst_shaders::atlas_frag_spv_refl,
    const hut::shared_ref<proj_ubo> &, const hut::shared_atlas &, const hut::shared_sampler &>;

using atlas_mask = hut::pipeline<uint16_t, hut::tst_shaders::tex_mask_vert_spv_refl, hut::tst_shaders::atlas_mask_frag_spv_refl,
    const hut::shared_ref<proj_ubo> &, const hut::shared_atlas &, const hut::shared_sampler &>;

using box_rgba = hut::pipeline<uint16_t, hut::tst_shaders::box_rgba_vert_spv_refl, hut::tst_shaders::box_rgba_frag_spv_refl,
    const hut::shared_ref<proj_ubo> &>;

using rgb = hut::pipeline<uint16_t, hut::tst_shaders::rgb_vert_spv_refl, hut::tst_shaders::rgb_frag_spv_refl,
    const hut::shared_ref<proj_ubo> &>;

using rgb3d = hut::pipeline<uint16_t, hut::tst_shaders::rgb3d_vert_spv_refl, hut::tst_shaders::rgb_frag_spv_refl,
    const hut::shared_ref<vp_ubo>&>;

using rgba = hut::pipeline<uint16_t, hut::tst_shaders::rgba_vert_spv_refl, hut::tst_shaders::rgba_frag_spv_refl,
    const hut::shared_ref<proj_ubo> &>;

using skybox = hut::pipeline<uint16_t, hut::tst_shaders::skybox_vert_spv_refl, hut::tst_shaders::skybox_frag_spv_refl,
    const hut::shared_ref<vp_ubo>&, const hut::shared_image&, const hut::shared_sampler&>;

using tex = hut::pipeline<uint16_t, hut::tst_shaders::tex_vert_spv_refl, hut::tst_shaders::tex_frag_spv_refl,
    const hut::shared_ref<proj_ubo> &, const hut::shared_image &, const hut::shared_sampler &>;

using tex_rgb = hut::pipeline<uint16_t, hut::tst_shaders::tex_rgb_vert_spv_refl, hut::tst_shaders::tex_rgb_frag_spv_refl,
    const hut::shared_ref<proj_ubo> &, const hut::shared_image &, const hut::shared_sampler &>;

using tex_rgba = hut::pipeline<uint16_t, hut::tst_shaders::tex_rgba_vert_spv_refl, hut::tst_shaders::tex_rgba_frag_spv_refl,
    const hut::shared_ref<proj_ubo> &, const hut::shared_image &, const hut::shared_sampler &>;

using tex_mask = hut::pipeline<uint16_t, hut::tst_shaders::tex_mask_vert_spv_refl, hut::tst_shaders::tex_mask_frag_spv_refl,
    const hut::shared_ref<proj_ubo> &, const hut::shared_image &, const hut::shared_sampler &>;
