fix:
- can't grow a device local buffer referenced in some descriptor (buffer id changed)

refactor:
- texture atlas (decouple from font code)
- isolate xcb code in shared library

features:
- 9patch
- shader for rounded rectanle box, with drop shadow
    https://www.shadertoy.com/view/MddcW2
    http://madebyevan.com/shaders/fast-rounded-rectangle-shadows/
- automatic animation of ref (like blending from one tex_rgba::instance to another)
- scene/render tree, for ui hierarchy
- support more WSI

build:
- compile against a statically compiled harfbuzz?
