#<cldoc:index>

libhut: hobby moden-C++ UI toolkit library.

#<cldoc:References and TODOs>

* Demos
    * http://cgit.freedesktop.org/mesa/demos/tree/src/egl/opengles2/es2tri.c
    * http://cgit.freedesktop.org/wayland/weston/tree/clients/simple-egl.c

* Math
    * http://www.reedbeta.com/blog/2013/12/28/on-vector-math-libraries/
    * http://www.j3d.org/matrix_faq/matrfaq_latest.html

* Lines
    * http://artgrammer.blogspot.com/search/label/opengl

* PorterDuff
    * http://www.quake-1.com/docs/blending.jpg
    * http://image.slidesharecdn.com/08blend-120215155211-phpapp02/95/cs-354-blending-compositing-antialiasing-44-728.jpg?cb=1329343056
        * in http://slideshare.net/Mark_Kilgard/08blend

* Text
    * https://github.com/rougier/freetype-gl
    * http://http.developer.nvidia.com/GPUGems3/gpugems3_ch25.html
    * https://github.com/anoek/ex-sdl-cairo-freetype-harfbuzz/blob/master/ex-sdl-cairo-freetype-harfbuzz.c

* Windows
    * https://code.google.com/p/angleproject/

#<cldoc:Architecture>

* vec/mat

* utils
    * event: list of callbacks

* app
    * Port specific main loop and entry point from various app lifecycle
        * Dispatch system events to window objects
        * Dispatch system app events (pause, stop, ...) to the app
    * Provides info about the device and hardware capabilities
    * Creates windows

* buffer/texture
    * Holds data on GPU side

* drawable
    * Holds everything needed for a draw:
        * Draw mode (points, line, polygons, triangle strip..)
        * Draw size for lines/points
        * Blend mode
        * Vertices' buffer and offsets
        * Indices' buffer and offsets (optional)
        * Shader program and it's data
    * Shader code can't be uploaded but the code is generated dynamically depending on the optional features requested
    * You can define multiple times thus features to the drawable factory it's the GPU that will blend the different color layer you configured
        * Transform position by uniform matrix
        * Color uniform/attribute
        * Texture
        * Gradients

* surface: used to draw meshs, 3 main implementations:
    * window
        * Visible surface, with input events
    * FBO
        * Render to in-GPU texture
    * batch
        * Record mesh draw at low level, to replay them later
        * You can asks an optimization pass on the low level commands produced (reordering, etc to minimize state changes)

* bitmap: cpu side texture data

* bitmap_factory
    * Supports only PNG and JPEG
    * Support for async load, reusing

* canvas
     * Implements surface
     * 2D drawing based on HTML5's Canvas: paths, 
     * Texture atlas
     * Text rendering
     * Returns a batch

* ui
    * view/view_group
    * view has an attached theme
    * view_group has an attached layouter: linear_layout, grid_layout, table_layout, flexbox_layout
    * recycler_view & adapters
    * scroll_view, text_view, image_view, button_view, edit_view
    * video_view (ffmpeg?), web_view (cef?)

* material
    * material design views&groups (https://www.google.com/design/spec/components/bottom-sheets.html#bottom-sheets-content)

#<cldoc:Conventions>

* Matrices are row major
* Origin is top-left, surface units (position/size) are pixels in float, density is in dpi, angles in degrees
* All parameters that takes a color are RGBA vec4
* Supported pixel formats are RGBA_8888, RGBA_4444, RGB_565, LA_88, L_8
* Some custom units, with user literal operators: deg, rad, px, dp, sp, rel