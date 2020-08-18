This is a hobby attempt at a GUI library, still in early and unusable state (see todo.md).

Dependencies (Fedora):
- common: mesa-vulkan-devel, glm-devel, libpng-devel, freetype-devel, harfbuzz-devel, KhronosGroup/glslang.
- testing: gtest-devel.
- profling: fmt-devel, boost-devel.
- wayland backend: wayland-devel, libxkbcommon-devel, wayland-protocols-devel, wayland-cursor.
- xcb backend: libxcb-devel, xcb-util-devel, xcb-util-keysyms-devel, xcb-util-cursor-devel, xcb-util-wm-devel.
- mingw64 cross compiling: wine, mingw64-gcc-c++, mingw64-filesystem, mingw64-vulkan-headers, mingw64-vulkan-loader,
mingw64-libpng, mingw64-freetype, mingw64-harfbuzz, mingw64-boost.
    - link your host glm include dir to mingw sysroot include (`sudo ln -s /usr/include/glm /usr/x86_64-w64-mingw32/sys-root/mingw/include/glm`)
    - add your mingw sysroot bin to WINEPATH (`export WINEPATH=/usr/x86_64-w64-mingw32/sys-root/mingw/bin/`)
    - run with `mingw64-cmake <hut_root>`
