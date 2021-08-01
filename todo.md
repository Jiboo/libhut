fix:
  - [w32] ctrl/shift KEYUP event sent two times
  - [w32] delayed clipboard rendering not always working
  - [w32] se/sw cursors are the same as ne/nw (in the sense that NESW looks more like a NE, than a NESW, might be https://github.com/wine-mirror/wine/blob/96a6053feda4e16480c21d01b3688a8d38e5ad6d/dlls/winex11.drv/mouse.c#L834)
  - [wl] test xdg-decorations (not available on fedora 31 desktop/weston)
  - [wl] next configure after an maximize(false) is [0, 0] instead of size prior to maximize (same with fullscreen)
  - [wl] we probably want a resize event at window creation, before any draw event

refactor:
  - [wl] async clipboard read/write
  - make libpng optional, and move png loading function to an ext lib
  - make freetype/harfbuzz (along with hut's font/shaping) optional, and move them to an ext lib

features:
  - [vk] specialization constants for shaders (pipeline params?)
  - [w32] transparent window support
  - [wl][w32] clipboard image support
  - [w32] drap&drop URI/text

someday:
  - COLRv1 as an alternative to CPAL/COLR emoji fonts, but not sure a renderer will be available in freetype https://github.com/googlefonts/colr-gradients-spec/
  - WSL has a wayland server, apparently, test and consider removing the w32 backend altogether.
