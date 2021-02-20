#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>

#include <spirv_reflect.h>

#include "hut/utils.hpp"

using namespace std;
using namespace filesystem;
using namespace chrono;
using namespace hut;

using svmatch = match_results<string_view::const_iterator>;
using svsub_match = sub_match<string_view::const_iterator>;

inline string_view get_sv(const svsub_match& m) {
  return string_view(m.first, m.length());
}

inline bool regex_match(string_view sv,
                        svmatch& m,
                        const regex& e,
                        regex_constants::match_flag_type flags =
                        regex_constants::match_default) {
  return regex_match(sv.begin(), sv.end(), m, e, flags);
}

inline bool regex_match(string_view sv,
                        const regex& e,
                        regex_constants::match_flag_type flags =
                        regex_constants::match_default) {
  return regex_match(sv.begin(), sv.end(), e, flags);
}

string_view descriptor_type_vk(SpvReflectDescriptorType _type) {
  switch(_type) {
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
    default: throw runtime_error(sstream("unknown descriptor type ") << _type);
  }
}

string_view format_name_vk(SpvReflectFormat _format) {
  switch(_format) {
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return "VK_FORMAT_R32G32_SFLOAT";
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return "VK_FORMAT_R32G32B32_SFLOAT";
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return "VK_FORMAT_R32G32B32A32_SFLOAT";
    default: throw runtime_error(sstream("unknown format ") << _format);
  }
}

string_view format_name_glsl(SpvReflectFormat _format, int _columns) {
  switch (_columns) {
    case 0:
    case 1:
      switch(_format) {
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return "vec2";
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return "vec3";
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return "vec4";
        default: throw runtime_error(sstream("unknown format ") << _format);
      } break;
    case 2:
      switch(_format) {
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return "mat2x2";
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return "mat2x3";
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return "mat2x4";
        default: throw runtime_error(sstream("unknown format ") << _format);
      } break;
    case 3:
      switch(_format) {
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return "mat3x2";
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return "mat3x3";
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return "mat3x4";
        default: throw runtime_error(sstream("unknown format ") << _format);
      } break;
    case 4:
      switch(_format) {
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return "mat4x2";
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return "mat4x3";
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return "mat4x4";
        default: throw runtime_error(sstream("unknown format ") << _format);
      } break;
    default: throw runtime_error(sstream("unexpected column count ") << _columns);
  }
}

void reflect_bindings(ostream &_os, const SpvReflectShaderModule &_mod) {
  uint32_t bindings_count = 0;
  SpvReflectResult result = spvReflectEnumerateDescriptorBindings(&_mod, &bindings_count, nullptr);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);
  vector<SpvReflectDescriptorBinding*> bindings(bindings_count);
  result = spvReflectEnumerateDescriptorBindings(&_mod, &bindings_count, bindings.data());
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  _os << "  constexpr static std::array<VkDescriptorSetLayoutBinding, " << bindings_count
      << "> descriptor_bindings_ {\n";
  for (auto *binding : bindings) {
    _os << "    VkDescriptorSetLayoutBinding{.binding = " << binding->binding
        << ", .descriptorType = " << descriptor_type_vk(binding->descriptor_type)
        << ", .descriptorCount = " << binding->count
        << ", .stageFlags = " << (_mod.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT ? "VK_SHADER_STAGE_VERTEX_BIT" : "VK_SHADER_STAGE_FRAGMENT_BIT")
        << ", .pImmutableSamplers = nullptr},\n";
  }
  _os << "  };" << endl;
}

// FIXME JBL: string_view for _extra_offset_op when supported by my local and mingw32 STLs
void reflect_vertex_input(ostream &_os, SpvReflectInterfaceVariable *_input, int _binding, string_view _name, int _location_offset = 0, string _extra_offset_op = "") {
  _os << "    VkVertexInputAttributeDescription{.location = " << (_input->location + _location_offset)
      << ", .binding = " << _binding
      << ", .format = " << format_name_vk(_input->format)
      << ", .offset = offsetof(" << (_binding == 0 ? "vertex" : "instance") << ", " << _name << "_)" << _extra_offset_op
      << "},\n";
}

void reflect_vertex_inputs(ostream &_os, const SpvReflectShaderModule &_mod) {
  uint32_t inputs_count = 0;
  SpvReflectResult result = spvReflectEnumerateInputVariables(&_mod, &inputs_count, nullptr);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);
  vector<SpvReflectInterfaceVariable*> inputs(inputs_count);
  result = spvReflectEnumerateInputVariables(&_mod, &inputs_count, inputs.data());
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  std::sort(inputs.begin(), inputs.end(), [](const auto *l, const auto *r) {
    return l->location < r->location;
  });

  struct vertex_data_member {
    SpvReflectInterfaceVariable *input_;
    string_view name_;

    vertex_data_member(SpvReflectInterfaceVariable *_input, string_view _name) : input_(_input), name_(_name) {}
  };
  vector<vertex_data_member> vertex_inputs, instance_inputs;

  /* FIXME JBL: Can't found something better to do than to use naming convention for vertex/instance split
   * "binding" decoration is forbidden, and apparently can't use SPIRV-Reflect for custom decorations
   * or pragmas for cpp-like "attributes"..*/

  constexpr string_view input_regex_source = "in_(v|i)_([a-z0-9_]*)";
  static regex input_regex(input_regex_source.data(), input_regex_source.size());
  static svmatch input_match;

  std::vector<std::string> attributes;
  ostringstream attribute_buffer;

  for (auto *input : inputs) {
    try {
      if (input->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
        continue;

      int binding = 0;
      if (!regex_match(string_view{input->name}, input_match, input_regex)) {
        throw runtime_error(hut::sstream("did not match regexp ") << input_regex_source);
      }
      else {
        auto short_name = string_view{input_match[2].first, input_match[2].second};
        if (input_match[1] == "i") {
          instance_inputs.emplace_back(input, short_name);
          binding = 1;
        }
        else {
          vertex_inputs.emplace_back(input, short_name);
        }

        auto columns = input->numeric.matrix.column_count;
        if (columns == 0) {
          attribute_buffer.str("");
          reflect_vertex_input(attribute_buffer, input, binding, short_name);
          attributes.emplace_back(attribute_buffer.str());
        }
        else {
          for (int i = 0; i < columns; i++) {
            attribute_buffer.str("");
            reflect_vertex_input(attribute_buffer, input, binding, short_name, i, sstream(" + sizeof(") << format_name_glsl(input->format, 0) << ") * " << i);
            attributes.emplace_back(attribute_buffer.str());
          }
        }
      }
    }
    catch(const exception &_ex) {
      throw runtime_error(sstream("while reflecting on field ") << input->name << ": " << _ex.what());
    }
  }

  _os << "\n  struct vertex {\n";
  for (auto &member : vertex_inputs) {
    _os << "    " << format_name_glsl(member.input_->format, member.input_->numeric.matrix.column_count)
        << " " << member.name_ << "_;\n";
  }
  _os << "  }; // struct vertex\n";

  _os << "\n  struct instance {\n";
  for (auto &member : instance_inputs) {
    _os << "    " << format_name_glsl(member.input_->format, member.input_->numeric.matrix.column_count)
        << " " << member.name_ << "_;\n";
  }
  _os << "  }; // struct instance\n";

  _os << "\n  constexpr static std::array<VkVertexInputAttributeDescription, " << attributes.size()
      << "> vertices_description_ {\n";
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

int main(int argc, char **argv) {
  auto start = steady_clock::now();

  for (auto i = 0; i < argc; i++)
    cout << argv[i] << " ";
  cout << endl;

  if (argc <= 3)
    throw runtime_error(sstream("usage: ") << argv[0] << " <namespace> <output> <list of input...>");

  path output_path = argv[2];
  if (output_path.has_parent_path() && !exists(output_path.parent_path()))
    create_directories(output_path.parent_path());

  path output_base_path = output_path / argv[1];
  path output_h_path = output_base_path;
  output_h_path += ".hpp";

  ofstream output_h(output_h_path, ios::out | ios::trunc);
  if (!output_h.is_open())
    throw runtime_error(sstream("can't open ") << output_h_path << ": " << strerror(errno));

  auto bundle_namespace = string(argv[1]);
  assert(bundle_namespace.ends_with("_refl"));
  bundle_namespace.resize(bundle_namespace.size() - strlen("_refl"));

  output_h << "// This is an autogenerated file.\n"
            "#pragma once\n"
            "#include <array>\n"
            "#include <cstdint>\n"
            "#include <vulkan/vulkan.h>\n"
            "#include \"" << bundle_namespace << ".hpp\"\n"
            "namespace hut::" << bundle_namespace<< " {\n";

  for (auto i = 3; i < argc; i++) {
    path input_path = argv[i];

    ifstream input(input_path, ios::ate | ios::binary);
    if (!input.is_open())
      throw runtime_error(sstream("can't open ") << input_path << ": " << strerror(errno));

    string symbol = input_path.filename().string();
    replace(symbol.begin(), symbol.end(), '.', '_');
    replace(symbol.begin(), symbol.end(), '-', '_');

    auto found_size = input.tellg();
    auto written = 0;

    input.seekg(0);
    string spirv_data;
    spirv_data.resize(found_size);
    input.read(spirv_data.data(), spirv_data.size());
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_data.size(), spirv_data.data(), &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    output_h << "\nstruct " << symbol << "_refl {\n";

    try {
      switch (module.shader_stage) {
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: reflect_vertex_shader(output_h, module); break;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: reflect_fragment_shader(output_h, module); break;
        default:
          throw runtime_error(sstream("reflecing on unsuported vertex stage ") << module.shader_stage);
      }
    }
    catch(const exception &_ex) {
      throw runtime_error(sstream("exception while reflecing on ") << input_path << ": " << _ex.what());
    }

    output_h << "\n  constexpr static auto &bytecode_ = " << bundle_namespace << "::" << symbol << ";\n";

    output_h << "}; // struct " << symbol << "_refl\n";

    spvReflectDestroyShaderModule(&module);
  }

  output_h << "}  // namespace hut::" << argv[1] << '\n';

  output_h.flush();

  cout << "Generated " << argv[1] << " at " << output_path << " in "
            << duration<double, milli>(steady_clock::now() - start).count() << "ms." << endl;
}
