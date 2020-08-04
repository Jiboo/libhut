fix:

refactor:
- have the staging buffer always mapped?
- texture atlas (decouple from font code)
- isolate (same?) platform code (like xcb vs wayland) in shared library that could be loaded at runtime
- buffer allocation strategies/algorithms
    - the staging buffer strategy could be much simpler, as it is cleared at once when submitted, fragmentation isn't a problem

features:
- multisampling
- clipboard
- drap&drop URI/text

build:
- compile against a statically compiled harfbuzz?
