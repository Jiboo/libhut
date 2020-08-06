fix:
- [w32] ctrl/shift KEYUP event sent two times

refactor:
- have the staging buffer always mapped?
- texture atlas (decouple from font code)
- isolate (same?) platform code (like xcb vs wayland) in shared library that could be loaded at runtime
- buffer allocation strategies/algorithms
    - the staging buffer strategy could be much simpler, as it is cleared at once when submitted, fragmentation isn't a problem
- [wl] async clipboard read/write

features:
- multisampling
- [xcb][w32] clipboard
- [wl][xcb][w32] drap&drop URI/text
