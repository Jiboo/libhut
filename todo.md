fix:
  - [w32] ctrl/shift KEYUP event sent two times
  - [w32] delayed clipboard rendering not always working
  - [w32] se/sw cursors are the same as ne/nw (in the sense that NESW looks more like a NE, than a NESW, might be https://github.com/wine-mirror/wine/blob/96a6053feda4e16480c21d01b3688a8d38e5ad6d/dlls/winex11.drv/mouse.c#L834)
  - [wl] test xdg-decorations (not available on fedora 31 desktop/weston)
  - [wl] next configure after an maximize(false) is [0, 0] instead of size prior to maximize (same with fullscreen)
  - [wl] we probably want a resize event at window creation, before any draw event

refactor:
  - [wl] async clipboard read/write
  - ~~atlas => atlas_pool (same change a buffer => buffer_pool), a collection of images, instead of a single one~~
        - ~~not with multiple layers, cause we can't add new layers without creating a new image~~
        - ~~any extension to do that?~~
      - ~~add a ref like buffer_pool, allowing to update pixels, and free this space when ref is destroyed~~
        - rework load_png to target an atlas_pool
    - rework pipelines
      - simplify by removing attachments customisation, make only ONE atlas_pool and UBO bindable (no version without UBO or atlas, but support for them being empty refs)
      - make sure we update descriptors before usage in on_draw (and no way to update them before draw finished)
        - if the atlas maintain a list of pipelines using it, it could potentially post a "staging" job to update the descriptor sets, when a new "layer" is added to the atlas
      - ~~make descriptorBindingPartiallyBound requried (100% on gpuinfo), and pass 256 textures, bound from the atlas~~
      - ~~make shaderSampledImageArrayNonUniformIndexing required (100% on gpuinfo), and allow indexing into atlas textures array from shader~~
      - https://github.com/KhronosGroup/Vulkan-Guide/blob/master/chapters/extensions/VK_EXT_descriptor_indexing.md
      - http://chunkstories.xyz/blog/a-note-on-descriptor-indexing/
    - huge, but will make image as easy to work with as the buffer, just create a pool and alloc from it.

features:
  - [w32] transparent window support
  - [wl][w32] clipboard image support
  - [w32] drap&drop URI/text
