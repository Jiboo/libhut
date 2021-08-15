fix:
  - we probably want a resize event at window creation, before any draw event?
  - xdg_toplevel_resize seems to ignore xdg_toplevel_set_min/max_size (repro on mutter and weston)

refactor: 
  - make freetype/harfbuzz (along with hut's font/shaping) optional, and move them to an ext lib

features:
  - specialization constants for shaders reflection (if not in spirv-reflect, maybe move to spirv-cross)
  - [imgdec] support more codecs than png, possible avif hardware decode through vulkan video?
  - [imgdec] add streaming and progressive decoding APIs
  - moar unit tests
  - some way to automate gui testing, simulate inputs and checks framebuffer?
