fix:
- [w32] ctrl/shift KEYUP event sent two times
- [w32] delayed clipboard rendering not always working
- [wl] test xdg-decorations (not available on fedora 31 desktop/weston)
- [wl] next configure after an unset_maximize would be [0, 0] instead of size prior to maximize

refactor:
- texture atlas (decouple from font code)
- isolate (same?) platform code (like xcb vs wayland) in shared library that could be loaded at runtime
- buffer allocation strategies/algorithms
    - the staging buffer strategy could be much simpler, as it is cleared at once when submitted, fragmentation isn't a problem
    - have the staging buffer always mapped (would avoid having to lock staging_mutex_ while loading png for example)
- [wl] async clipboard read/write

features:
- fullscreen
- multisampling
- clipboard image
- [xcb] clipboard
- [wl][xcb][w32] drap&drop URI/text
- gpu timestamps profiling for command buffer execution (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkCmdWriteTimestamp.html)
- instant and metadata profiling events
