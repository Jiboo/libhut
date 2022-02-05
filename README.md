libhut is library a vulkan & wayland wrapper.
You can use optional extensions, that can be configured at build time with cmake options.

This is a hobby project, expect building nightmare, runtime crashes, missing features/documentation/tests,
although contributions are welcome.

Available extensions:

- imgdec: decode various images and upload them to gpu easily
- imgui: DearImGui backend implementation for libhut
- ktx2: minimal ktx2 support (no basisu or supercompression)
- render2d: axis-aligned box drawing
- renderdoc: allows triggering renderdoc captures programmatically, useful for headless unittests
- text: text layout and rendering
- ui: lightweight ui toolkit using render2d and text extensions

Dependencies (Fedora):

- common: mesa-vulkan-devel, glm-devel.
- wayland: wayland-devel, wayland-protocols-devel, libwayland-cursor, libxkbcommon-devel.
- build time: glslang (KhronosGroup/glslang), KhronosGroup/SPIRV-Reflect (via submodule).
- testing (optional): gtest-devel.
- profiling (optional): fmt-devel, boost-devel.

Extensions dependencies (all optional):

- imgdec: randy408/libspng (via submodule).
- imgui: ocornut/imgui (via submodule).
- renderdoc: renderdoc-devel.
- text extension: freetype-devel, harfbuzz-devel.
