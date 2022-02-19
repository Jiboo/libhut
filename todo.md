fix:

- we probably want a resize event at window creation, before any draw event?
- xdg_toplevel_resize seems to ignore xdg_toplevel_set_min/max_size (repro on mutter and weston)
- gnome rescales my window during overlap between screens with different scales, unsure how to detect it and trigger
  on_scale at that time, as we get a wl_surface.enter as soon as there is overlap but the gnome scaling happens when
  like more than 50% of the window is on a different scale.

refactor:

- refactor buffer into two separate concepts, to have symmetry with atlas vs image classes.
- optimize staging to batch them, possibly using VK_KHR_copy_commands2 for staging with same src&dst.

features:

- specialization constants support for shaders reflection
  * https://github.com/KhronosGroup/SPIRV-Reflect/issues/110
- push constants support for shaders reflection
- [imgdec] support more codecs than png, possible avif hardware decode through vulkan video?
- [imgdec] add streaming and progressive decoding APIs
- moar unit tests
- some way to automate gui testing, simulate inputs and checks framebuffer?
