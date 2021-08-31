fix:
  - we probably want a resize event at window creation, before any draw event?
  - xdg_toplevel_resize seems to ignore xdg_toplevel_set_min/max_size (repro on mutter and weston)
  - text renderer is making renderdoc crash, I'm probably doing something wrong

refactor: 
  - (nothing yet)

features:
  - specialization constants support for shaders reflection
    - https://github.com/KhronosGroup/SPIRV-Reflect/issues/110
  - push constants support for shaders reflection
  - [imgdec] support more codecs than png, possible avif hardware decode through vulkan video?
  - [imgdec] add streaming and progressive decoding APIs
  - moar unit tests
  - some way to automate gui testing, simulate inputs and checks framebuffer?
