Using the cmake command gen_spv_bundle will compile and embed the SPIRV code, as well as some reflection info necessary
to setup the pipeline correctly.

As there is no way to set custom attribute (as in C++), or other custom annotations on inputs, you might have to add
some prefix/suffix in the inputs name:

- Source prefix, `in_i_` or `in_v_` to distinguish attributes that are fed by the attribute or instance buffer.
- Format suffix, allows to override the default format for an input, for example:
  `in vec4 in_v_col_r8g8b8a8_unorm;` will result as
  `VkVertexInputAttributeDescription{.format = VK_FORMAT_R8G8B8A8_UNORM ...}`
  in the reflected info, but without the format specifier, it would be a `VK_FORMAT_R32G32B32A32_SFLOAT` which is the
  implicit default for vec4.
