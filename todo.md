fix:
- [w32] ctrl/shift KEYUP event sent two times
- [w32] delayed clipboard rendering not always working
- [wl] test xdg-decorations (not available on fedora 31 desktop/weston)
- [wl] next configure after an unset_maximize would be [0, 0] instead of size prior to maximize

refactor:
- texture atlas (decouple from font code)
- [wl] async clipboard read/write

features:
- fullscreen
- multisampling
- clipboard image
- [xcb] clipboard
- [wl][xcb][w32] drap&drop URI/text
- gpu timestamps profiling for command buffer execution (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkCmdWriteTimestamp.html)
- instant and metadata profiling events
