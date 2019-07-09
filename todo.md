fix:

refactor:
- texture atlas (decouple from font code)
- isolate xcb code in shared library
- buffer allocation strategies
    the staging buffer strategy could be much simpler, as it is cleared at once, and not fragment per fragment

features:
- 9patch
- shader for rounded rectanle box, with drop shadow
    https://www.shadertoy.com/view/MddcW2
    http://madebyevan.com/shaders/fast-rounded-rectangle-shadows/
- automatic animation of ref (like blending from one tex_rgba::instance to another)
- scene/render tree, for ui hierarchy
- support more WSI
- perf stats

build:
- compile against a statically compiled harfbuzz?
