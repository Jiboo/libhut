#include <cassert>
#include <cstring>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

#include <spirv_reflect.h>

#include "hut/utils/chrono.hpp"
#include "hut/utils/glm.hpp"
#include "hut/utils/sstream.hpp"

using namespace std;
using namespace filesystem;
using namespace chrono;
using namespace hut;

using svmatch     = match_results<string_view::const_iterator>;
using svsub_match = sub_match<string_view::const_iterator>;

inline string_view get_sv(const svsub_match &_m) {
  return {_m.first, static_cast<string_view::size_type>(_m.length())};
}

inline bool regex_match(string_view _sv, svmatch &_m, const regex &_e,
                        regex_constants::match_flag_type _flags = regex_constants::match_default) {
  return regex_match(_sv.begin(), _sv.end(), _m, _e, _flags);
}

inline bool regex_match(string_view _sv, const regex &_e,
                        regex_constants::match_flag_type _flags = regex_constants::match_default) {
  return regex_match(_sv.begin(), _sv.end(), _e, _flags);
}

string_view descriptor_type_vk(SpvReflectDescriptorType _type) {
  switch (_type) {
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
    default: throw runtime_error(sstream("unknown descriptor type ") << _type);
  }
}

constexpr string_view VK_FORMATS[] = {
    "r4g4_unorm_pack8",
    "r4g4b4a4_unorm_pack16",
    "b4g4r4a4_unorm_pack16",
    "r5g6b5_unorm_pack16",
    "b5g6r5_unorm_pack16",
    "r5g5b5a1_unorm_pack16",
    "b5g5r5a1_unorm_pack16",
    "a1r5g5b5_unorm_pack16",
    "r8_unorm",
    "r8_snorm",
    "r8_uscaled",
    "r8_sscaled",
    "r8_uint",
    "r8_sint",
    "r8_srgb",
    "r8g8_unorm",
    "r8g8_snorm",
    "r8g8_uscaled",
    "r8g8_sscaled",
    "r8g8_uint",
    "r8g8_sint",
    "r8g8_srgb",
    "r8g8b8_unorm",
    "r8g8b8_snorm",
    "r8g8b8_uscaled",
    "r8g8b8_sscaled",
    "r8g8b8_uint",
    "r8g8b8_sint",
    "r8g8b8_srgb",
    "b8g8r8_unorm",
    "b8g8r8_snorm",
    "b8g8r8_uscaled",
    "b8g8r8_sscaled",
    "b8g8r8_uint",
    "b8g8r8_sint",
    "b8g8r8_srgb",
    "r8g8b8a8_unorm",
    "r8g8b8a8_snorm",
    "r8g8b8a8_uscaled",
    "r8g8b8a8_sscaled",
    "r8g8b8a8_uint",
    "r8g8b8a8_sint",
    "r8g8b8a8_srgb",
    "b8g8r8a8_unorm",
    "b8g8r8a8_snorm",
    "b8g8r8a8_uscaled",
    "b8g8r8a8_sscaled",
    "b8g8r8a8_uint",
    "b8g8r8a8_sint",
    "b8g8r8a8_srgb",
    "a8b8g8r8_unorm_pack32",
    "a8b8g8r8_snorm_pack32",
    "a8b8g8r8_uscaled_pack32",
    "a8b8g8r8_sscaled_pack32",
    "a8b8g8r8_uint_pack32",
    "a8b8g8r8_sint_pack32",
    "a8b8g8r8_srgb_pack32",
    "a2r10g10b10_unorm_pack32",
    "a2r10g10b10_snorm_pack32",
    "a2r10g10b10_uscaled_pack32",
    "a2r10g10b10_sscaled_pack32",
    "a2r10g10b10_uint_pack32",
    "a2r10g10b10_sint_pack32",
    "a2b10g10r10_unorm_pack32",
    "a2b10g10r10_snorm_pack32",
    "a2b10g10r10_uscaled_pack32",
    "a2b10g10r10_sscaled_pack32",
    "a2b10g10r10_uint_pack32",
    "a2b10g10r10_sint_pack32",
    "r16_unorm",
    "r16_snorm",
    "r16_uscaled",
    "r16_sscaled",
    "r16_uint",
    "r16_sint",
    "r16_sfloat",
    "r16g16_unorm",
    "r16g16_snorm",
    "r16g16_uscaled",
    "r16g16_sscaled",
    "r16g16_uint",
    "r16g16_sint",
    "r16g16_sfloat",
    "r16g16b16_unorm",
    "r16g16b16_snorm",
    "r16g16b16_uscaled",
    "r16g16b16_sscaled",
    "r16g16b16_uint",
    "r16g16b16_sint",
    "r16g16b16_sfloat",
    "r16g16b16a16_unorm",
    "r16g16b16a16_snorm",
    "r16g16b16a16_uscaled",
    "r16g16b16a16_sscaled",
    "r16g16b16a16_uint",
    "r16g16b16a16_sint",
    "r16g16b16a16_sfloat",
    "r32_uint",
    "r32_sint",
    "r32_sfloat",
    "r32g32_uint",
    "r32g32_sint",
    "r32g32_sfloat",
    "r32g32b32_uint",
    "r32g32b32_sint",
    "r32g32b32_sfloat",
    "r32g32b32a32_uint",
    "r32g32b32a32_sint",
    "r32g32b32a32_sfloat",
    "r64_uint",
    "r64_sint",
    "r64_sfloat",
    "r64g64_uint",
    "r64g64_sint",
    "r64g64_sfloat",
    "r64g64b64_uint",
    "r64g64b64_sint",
    "r64g64b64_sfloat",
    "r64g64b64a64_uint",
    "r64g64b64a64_sint",
    "r64g64b64a64_sfloat",
    "b10g11r11_ufloat_pack32",
    "e5b9g9r9_ufloat_pack32",
};

string_view remove_format_suffix(string_view _input) {
  for (const auto &vk_format : VK_FORMATS) {
    if (_input.ends_with(vk_format))
      return _input.substr(0, _input.size() - vk_format.size()
                                  - 1);  // NOTE JBL: extra one for the underscore, '_'<format>
  }
  return _input;
}

string format_name_vk(SpvReflectInterfaceVariable *_input) {
  if (_input->array.dims_count != 0)
    throw runtime_error(sstream("unsuported array type"));
  if (_input->array.stride != 0)
    throw runtime_error(sstream("unsuported type with stride"));
  if (_input->numeric.scalar.width == 0)
    throw runtime_error(sstream("unsuported type without width"));

  stringstream os;
  os << "VK_FORMAT_";

  const string_view name = _input->name;
  for (const auto &vk_format : VK_FORMATS) {
    if (name.ends_with(vk_format)) {
      for (auto c : vk_format)
        os << (char)std::toupper(c);
      return os.str();
    }
  }

  os << "R" << _input->numeric.scalar.width;
  if (_input->numeric.vector.component_count > 1)
    os << "G" << _input->numeric.scalar.width;
  if (_input->numeric.vector.component_count > 2)
    os << "B" << _input->numeric.scalar.width;
  if (_input->numeric.vector.component_count > 3)
    os << "A" << _input->numeric.scalar.width;

  switch (_input->format) {
    case SPV_REFLECT_FORMAT_R32_SFLOAT:
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
    case SPV_REFLECT_FORMAT_R64_SFLOAT:
    case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
    case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
    case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT: os << "_SFLOAT"; break;

    default:
      if (_input->numeric.scalar.width > 16)
        os << (_input->numeric.scalar.signedness != 0u ? "_SINT" : "_UINT");
      else
        os << "_UNORM";
      break;
  }

  return os.str();
}

string format_name_cpp(SpvReflectInterfaceVariable *_input, uint _columns = NUMAX<uint>) {
  _columns = (_columns == NUMAX<uint> ? _input->numeric.matrix.column_count : _columns);

  const string_view name = _input->name;
  string            format;
  for (const auto &vk_format : VK_FORMATS) {
    if (name.ends_with(vk_format)) {
      format = vk_format;
      break;
    }
  }

  if (format.empty()) {
    stringstream os;
    bool         fp = false;
    switch (_input->format) {
      case SPV_REFLECT_FORMAT_R32_SFLOAT:
      case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
      case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
      case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
        os << "f32";
        fp = true;
        break;
      case SPV_REFLECT_FORMAT_R64_SFLOAT:
      case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
      case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
      case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
        os << "f64";
        fp = true;
        break;

      default:
        os << (_input->numeric.scalar.signedness == 0 ? "u" : "i");
        os << _input->numeric.scalar.width;
        break;
    }
    os << "r" << _input->numeric.scalar.width;
    if (_input->numeric.vector.component_count > 1)
      os << "b" << _input->numeric.scalar.width;
    if (_input->numeric.vector.component_count > 2)
      os << "b" << _input->numeric.scalar.width;
    if (_input->numeric.vector.component_count > 3)
      os << "a" << _input->numeric.scalar.width;

    if (fp)
      os << "_sfloat";
    else if (_input->numeric.scalar.width > 16)
      os << (_input->numeric.scalar.signedness != 0u ? "_sint" : "_uint");
    else
      os << "_unorm";

    format = os.str();
  }

  if (format.ends_with("pack8"))
    return "u8";
  if (format.ends_with("pack16"))
    return "u16";
  else if (format.ends_with("pack32"))
    return "u32";

  stringstream os;
  if (format.ends_with("sint") || format.ends_with("snorm") || format.ends_with("sscaled"))
    os << "i";
  else if (format.ends_with("uint") || format.ends_with("unorm") || format.ends_with("uscaled"))
    os << "u";
  else if (format.ends_with("sfloat"))
    os << "f";
  else
    throw std::runtime_error(sstream("unknown suffix for ") << format);

  size_t occ8  = std::count(format.begin(), format.end(), '8');
  size_t occ16 = std::count(format.begin(), format.end(), '1');
  size_t occ32 = std::count(format.begin(), format.end(), '3');
  size_t occ64 = std::count(format.begin(), format.end(), '4');

  if (occ64 > 0)
    os << "64";
  else if (occ32 > 0)
    os << "32";
  else if (occ16 > 0)
    os << "16";
  else if (occ8 > 0)
    os << "8";

  if (_columns > 0) {
    os << "mat" << _columns;
    if (_input->numeric.matrix.row_count != _columns)
      os << "x" << _input->numeric.matrix.row_count;
  } else if (_input->numeric.vector.component_count > 0) {
    os << "vec" << _input->numeric.vector.component_count;
  }

  return os.str();
}

void reflect_bindings(ostream &_os, const SpvReflectShaderModule &_mod) {
  u32              bindings_count = 0;
  SpvReflectResult result         = spvReflectEnumerateDescriptorBindings(&_mod, &bindings_count, nullptr);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);
  vector<SpvReflectDescriptorBinding *> bindings(bindings_count);
  result = spvReflectEnumerateDescriptorBindings(&_mod, &bindings_count, bindings.data());
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  _os << "  constexpr static std::array<VkDescriptorSetLayoutBinding, " << bindings_count
      << "> DESCRIPTOR_BINDINGS {\n";
  for (auto *binding : bindings) {
    if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        && binding->type_description->op == SpvOpTypeRuntimeArray && binding->count == 1)
      throw std::runtime_error("missing size on sampler array (specify a dimension size), or 1 specified (if only one "
                               "is needed don't use an array)");
    _os << "    VkDescriptorSetLayoutBinding{.binding = " << binding->binding
        << ", .descriptorType = " << descriptor_type_vk(binding->descriptor_type)
        << ", .descriptorCount = " << binding->count << ", .stageFlags = "
        << (_mod.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT ? "VK_SHADER_STAGE_VERTEX_BIT"
                                                                     : "VK_SHADER_STAGE_FRAGMENT_BIT")
        << ", .pImmutableSamplers = nullptr},\n";
  }
  _os << "  };" << endl;
}

// FIXME JBL: string_view for _extra_offset_op when supported by my local and mingw32 STLs
void reflect_vertex_input(ostream &_os, SpvReflectInterfaceVariable *_input, uint _binding, string_view _name,
                          uint _location_offset = 0, const string &_extra_offset_op = "") {
  _os << "    VkVertexInputAttributeDescription{.location = " << (_input->location + _location_offset)
      << ", .binding = " << _binding << ", .format = " << format_name_vk(_input) << ", .offset = offsetof("
      << (_binding == 0 ? "vertex" : "instance") << ", " << _name << "_)" << _extra_offset_op << "},\n";
}

void reflect_vertex_inputs(ostream &_os, const SpvReflectShaderModule &_mod) {
  u32              inputs_count = 0;
  SpvReflectResult result       = spvReflectEnumerateInputVariables(&_mod, &inputs_count, nullptr);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);
  vector<SpvReflectInterfaceVariable *> inputs(inputs_count);
  result = spvReflectEnumerateInputVariables(&_mod, &inputs_count, inputs.data());
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  std::sort(inputs.begin(), inputs.end(), [](const auto *_l, const auto *_r) { return _l->location < _r->location; });

  struct vertex_data_member {
    SpvReflectInterfaceVariable *input_;
    string_view                  name_;

    vertex_data_member(SpvReflectInterfaceVariable *_input, string_view _name)
        : input_(_input)
        , name_(_name) {}
  };
  vector<vertex_data_member> vertex_inputs;
  vector<vertex_data_member> instance_inputs;

  /* FIXME JBL: Can't found something better to do than to use naming convention for vertex/instance split
   * "binding" decoration is forbidden, and apparently can't use SPIRV-Reflect for custom decorations
   * or pragmas for cpp-like "attributes"..*/

  constexpr string_view INPUT_REGEXP_SOURCE = "in_(v|i)_([a-z0-9_]*)";
  static regex          s_input_regex(INPUT_REGEXP_SOURCE.data(), INPUT_REGEXP_SOURCE.size());
  static svmatch        s_input_match;

  std::vector<std::string> attributes;
  ostringstream            attribute_buffer;

  for (auto *input : inputs) {
    try {
      if ((input->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) != 0u)
        continue;

      int binding = 0;
      if (!regex_match(string_view{input->name}, s_input_match, s_input_regex)) {
        throw runtime_error(sstream("did not match regexp ") << INPUT_REGEXP_SOURCE);
      }
      auto name_without_prefix = string_view{s_input_match[2].first, s_input_match[2].second};
      auto short_name          = remove_format_suffix(name_without_prefix);
      if (s_input_match[1] == "i") {
        instance_inputs.emplace_back(input, short_name);
        binding = 1;
      } else {
        vertex_inputs.emplace_back(input, short_name);
      }

      auto columns = input->numeric.matrix.column_count;
      if (columns == 0) {
        attribute_buffer.str("");
        reflect_vertex_input(attribute_buffer, input, binding, short_name);
        attributes.emplace_back(attribute_buffer.str());
      } else {
        for (uint i = 0; i < columns; i++) {
          attribute_buffer.str("");
          reflect_vertex_input(attribute_buffer, input, binding, short_name, i,
                               sstream(" + sizeof(") << format_name_cpp(input, 0) << ") * " << i);
          attributes.emplace_back(attribute_buffer.str());
        }
      }

    } catch (const exception &ex) {
      throw runtime_error(sstream("while reflecting on field ") << input->name << ": " << ex.what());
    }
  }

  _os << "\n  struct vertex {\n";
  for (auto &member : vertex_inputs) {
    _os << "    " << format_name_cpp(member.input_) << " " << member.name_ << "_;\n";
  }
  _os << "  }; // struct vertex\n";

  _os << "\n  struct instance {\n";
  for (auto &member : instance_inputs) {
    _os << "    " << format_name_cpp(member.input_) << " " << member.name_ << "_;\n";
  }
  _os << "  }; // struct instance\n";

  _os << "\n  constexpr static std::array<VkVertexInputAttributeDescription, " << attributes.size()
      << "> VERTICES_DESCRIPTION {\n";
  for (auto &attribute : attributes)
    _os << attribute;
  _os << "  };" << endl;
}

void reflect_fragment_shader(ostream &_os, const SpvReflectShaderModule &_mod) {
  reflect_bindings(_os, _mod);
}

void reflect_vertex_shader(ostream &_os, const SpvReflectShaderModule &_mod) {
  reflect_bindings(_os, _mod);
  reflect_vertex_inputs(_os, _mod);
}

int main(int _argc, char **_argv) {
  try {
    auto start = steady_clock::now();

    for (auto i = 0; i < _argc; i++)
      cout << _argv[i] << " ";
    cout << endl;

    if (_argc <= 3)
      throw runtime_error(sstream("usage: ") << _argv[0] << " <namespace> <output> <list of input...>");

    path output_path = _argv[2];
    if (output_path.has_parent_path() && !exists(output_path.parent_path()))
      create_directories(output_path.parent_path());

    path output_base_path = output_path / _argv[1];
    path output_h_path    = output_base_path;
    output_h_path += ".hpp";

    ofstream output_h(output_h_path, ios::out | ios::trunc);
    if (!output_h.is_open())
      throw runtime_error(sstream("can't open ") << output_h_path << ": " << strerror(errno));

    auto bundle_namespace = string(_argv[1]);
    assert(bundle_namespace.ends_with("_refl"));
    bundle_namespace.resize(bundle_namespace.size() - strlen("_refl"));

    output_h << "// This is an autogenerated file.\n"
                "#pragma once\n"
                "#include <cstdint>\n"
                "#include <array>\n"
                "#include <vulkan/vulkan.h>\n"
                "#include \""
             << bundle_namespace
             << ".hpp\"\n"
                "namespace hut::"
             << bundle_namespace << " {\n";

    for (auto i = 3; i < _argc; i++) {
      path input_path = _argv[i];

      ifstream input(input_path, ios::ate | ios::binary);
      if (!input.is_open())
        throw runtime_error(sstream("can't open ") << input_path << ": " << strerror(errno));

      string filename = input_path.filename();
      string symbol   = filename;
      replace(symbol.begin(), symbol.end(), '.', '_');
      replace(symbol.begin(), symbol.end(), '-', '_');

      auto found_size = input.tellg();

      input.seekg(0);
      string spirv_data;
      spirv_data.resize(found_size);
      input.read(spirv_data.data(), static_cast<streamsize>(spirv_data.size()));
      SpvReflectShaderModule module;
      SpvReflectResult       result = spvReflectCreateShaderModule(spirv_data.size(), spirv_data.data(), &module);
      assert(result == SPV_REFLECT_RESULT_SUCCESS);

      output_h << "\nstruct " << symbol << "_refl {\n";

      try {
        switch (module.shader_stage) {
          case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: reflect_vertex_shader(output_h, module); break;
          case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: reflect_fragment_shader(output_h, module); break;
          default: throw runtime_error(sstream("reflecting on unsupported vertex stage ") << module.shader_stage);
        }
      } catch (const exception &ex) {
        throw runtime_error(sstream("exception while reflecting on ") << input_path << ": " << ex.what());
      }

      output_h << "\n  constexpr static auto &BYTECODE = " << bundle_namespace << "::" << symbol << ";\n";
      output_h << "\n  constexpr static auto FILENAME = \"" << filename << "\";\n";

      output_h << "}; // struct " << symbol << "_refl\n";

      spvReflectDestroyShaderModule(&module);
    }

    output_h << "}  // namespace hut::" << _argv[1] << '\n';

    output_h.flush();

    cout << "Generated " << _argv[1] << " at " << output_path << " in "
         << duration<double, milli>(steady_clock::now() - start).count() << "ms." << endl;

    return EXIT_SUCCESS;
  } catch (const std::exception &e) { std::cerr << "Caught expection: " << e.what() << std::endl; } catch (...) {
    std::cerr << "Caught unknown expection" << std::endl;
  }
  return EXIT_FAILURE;
}
