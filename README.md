HUT (Hobby Ui Toolkit), attempt at a GUI library, still in early and unusable state (see todo.md).

Wayland and Vulkan only.

Dependencies (Fedora):
- submodules: KhronosGroup/glslang (glsl->spirv), KhronosGroup/SPIRV-Reflect, imgui (optional, used for tests/playgrounds).
- common: mesa-vulkan-devel, glm-devel, libpng-devel, freetype-devel, harfbuzz-devel.
- testing: gtest-devel.
- profiling: fmt-devel, boost-devel.
- wayland: wayland-devel, libxkbcommon-devel, wayland-protocols-devel, libwayland-cursor.
