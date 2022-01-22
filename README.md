Hobby graphics and GUI library, Wayland and Vulkan only.

Dependencies (Fedora):

- common: mesa-vulkan-devel, glm-devel.
- wayland: wayland-devel, wayland-protocols-devel, libwayland-cursor, libxkbcommon-devel.
- build time: glslang (KhronosGroup/glslang), KhronosGroup/SPIRV-Reflect (via submodule).
- testing (optional): gtest-devel.
- profiling (optional): fmt-devel, boost-devel.

Extensions dependencies (all optional):

- imgdec extension: randy408/libspng (via submodule).
- imgui extension: ocornut/imgui (via submodule).
- renderdoc extension: renderdoc-devel.
- text extension: freetype-devel, harfbuzz-devel.
