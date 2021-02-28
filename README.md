HUT (Hobby Ui Toolkit), attempt at a GUI library, still in early and unusable state (see todo.md).

Dependencies (Fedora):
- submodules: KhronosGroup/glslang (glsl->spirv), KhronosGroup/SPIRV-Reflect, imgui (optional, used for tests/playgrounds).
- common: mesa-vulkan-devel, glm-devel, libpng-devel, freetype-devel, harfbuzz-devel.
- testing: gtest-devel.
- profling: fmt-devel, boost-devel.
- wayland backend: wayland-devel, libxkbcommon-devel, wayland-protocols-devel, wayland-cursor.
- mingw64 cross compiling: wine, mingw64-gcc-c++, mingw64-filesystem, mingw64-vulkan-headers, mingw64-vulkan-loader,
mingw64-libpng, mingw64-freetype, mingw64-harfbuzz.
  - link your host glm include dir to mingw sysroot include (`sudo ln -s /usr/include/glm /usr/x86_64-w64-mingw32/sys-root/mingw/include/glm`)
  - add your mingw sysroot bin to WINEPATH (`export WINEPATH=/usr/x86_64-w64-mingw32/sys-root/mingw/bin/`)
  - run with `mingw64-cmake <hut_root>`
  - You might need to manually fix glslang for https://github.com/KhronosGroup/glslang/issues/2460, remove GLSLANG_EXPORT on TObjectReflection::getType()
