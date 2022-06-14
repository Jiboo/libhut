hut/utils/glm.hpp imports the whole glm namespace inside hut, along with all glm utils, this also declares
basic GL-style numeric types: u16, f32, etc... as well as the vec<size, type> template and its aliases
u16vec2, f32vec4, etc.

hut/utils/length.hpp provides the unit (px, pt, ...) enumeration as well as the length<type, unit>
template and its vec variants aliases so that f32vec4_px is a vec<4, length<px, f32>>.

hut/utils/bbox.hpp provides an axis aligned bounding boxes template bbox<unit, type> and its aliases in the same
fashion as length, u16bbox_px is similar to an u16vec4_px, but with some more utilities, to compute its center,
size, etc.

hut/utils/color.hpp provides some strongly typed aliases for colors, u8vec4_rgba and f32vec4_rgba that you can
implicitly cast between these types.
