fix:
- threading, windows event pump needs to be on the same thread that called CreateWindow, so I had to do some hacks.
    The behaviour when resizing window might be buggy because of the current threading model.

refactor:
- texture atlas (decouple from font code)
- isolate (same?) platform code (like xcb vs wayland) in shared library that could be loaded at runtime
- buffer allocation strategies/algorithms
    the staging buffer strategy could be much simpler, as it is cleared at once, and not fragment per fragment
- prepare a frame on cpu while rendering another on gpu (note: would need to double all mutable buffer, as they shouldn't be modified while being used)

features:
- XCB paused/resumed
- 9patch
- automatic animation of ref (like blending from one rgba::instance to another)
- scene/render tree?, for ui hierarchy
- support more WSI
- perf stats

build:
- compile against a statically compiled harfbuzz?
