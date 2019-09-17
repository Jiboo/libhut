fix:

refactor:
- texture atlas (decouple from font code)
- isolate (same?) platform code (like xcb vs wayland) in shared library that could be loaded at runtime
- buffer allocation strategies/algorithms
    the staging buffer strategy could be much simpler, as it is cleared at once, and not fragment per fragment

features:
- XCB paused/resumed
- 9patch
- automatic animation of ref (like blending from one rgba::instance to another)
- scene/render tree?, for ui hierarchy
- multisampling

build:
- compile against a statically compiled harfbuzz?
