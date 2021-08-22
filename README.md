HUT (Hobby Ui Toolkit), attempt at a GUI library, still in early and unusable state (see todo.md).

Wayland and Vulkan only.

Dependencies (Fedora):
- common: mesa-vulkan-devel, glm-devel.
- wayland: wayland-devel, wayland-protocols-devel, libwayland-cursor, libxkbcommon-devel.
- build time: KhronosGroup/glslang (via submodule), KhronosGroup/SPIRV-Reflect (via submodule).
- testing (optional): gtest-devel.
- profiling (optional): fmt-devel, boost-devel.

Extensions dependencies (all optional):
- imgdec extension: spng-devel.
- imgui extension: ocornut/imgui (via submodule).
- renderdoc extension: renderdoc-devel.
- text extension: freetype-devel, harfbuzz-devel.
