fix:
  - next configure after an maximize(false) is [0, 0] instead of size prior to maximize (same with fullscreen)
  - we probably want a resize event at window creation, before any draw event

refactor: 
  - make libpng optional, and move png loading function to an ext lib
  - make freetype/harfbuzz (along with hut's font/shaping) optional, and move them to an ext lib

features:
  - clipboard image support
  - specialization constants for shaders reflection (if not in spirv-reflect, maybe move to spirv-cross)
