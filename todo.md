fix:
  - next configure after an maximize(false) is [0, 0] instead of size prior to maximize (same with fullscreen)
  - we probably want a resize event at window creation, before any draw event

refactor: 
  - make freetype/harfbuzz (along with hut's font/shaping) optional, and move them to an ext lib
  - move extension related unit tests to the extension directory

features:
  - clipboard image support
  - specialization constants for shaders reflection (if not in spirv-reflect, maybe move to spirv-cross)
  - [imgdec] support more codecs than png, possible avif hardware decode through vulkan video?
  - [imgdec] add streaming and progressive decoding APIs
