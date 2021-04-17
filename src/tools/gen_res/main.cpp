#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "hut/utils.hpp"

using namespace std;
using namespace std::filesystem;
using namespace std::chrono;
using namespace hut;

constexpr auto line_size = 0x10;

int main(int argc, char **argv) {
  auto start = steady_clock::now();

  if (argc <= 3)
    throw runtime_error(sstream("usage: ") << argv[0] << " <namespace> <output> <list of input...>");

  path output_path = argv[2];
  if (!exists(output_path))
    create_directories(output_path);

  path output_base_path = output_path / argv[1];
  path output_c_path = output_base_path;
  output_c_path += ".cpp";
  path output_h_path = output_base_path;
  output_h_path += ".hpp";

  ofstream output_c(output_c_path, ios::out | ios::trunc);
  if (!output_c.is_open())
    throw runtime_error(sstream("can't open ") << output_c_path << ": " << strerror(errno));

  ofstream output_h(output_h_path, ios::out | ios::trunc);
  if (!output_h.is_open())
    throw runtime_error(sstream("can't open ") << output_h_path << ": " << strerror(errno));

  output_h << "// This is an autogenerated file.\n"
            "#pragma once\n"
            "#include <array>\n"
            "#include <cstdint>\n"
            "namespace hut::" << argv[1] << " {\n";

  output_c << "// This is an autogenerated file.\n"
              "#include \"" << argv[1] << ".hpp\"\n\n"
              "namespace hut::" << argv[1] << " {\n";

  for (auto i = 3; i < argc; i++) {
    path input_path = argv[i];

    ifstream input(input_path, ios::ate | ios::binary);
    if (!input.is_open())
      throw runtime_error(sstream("can't open ") << input_path << ": " << strerror(errno));

    string symbol = input_path.filename().string();
    std::replace(symbol.begin(), symbol.end(), '.', '_');
    std::replace(symbol.begin(), symbol.end(), '-', '_');

    auto found_size = input.tellg();
    auto written = 0;

    output_h << "extern const std::array<uint8_t, " << dec << found_size << "> " << symbol << ";\n";

    output_c << "const std::array<uint8_t, " << dec << found_size << "> " << symbol << " = {\n";
    input.seekg(0);
    while (!input.eof()) {
      uint8_t line[line_size];
      input.read((char *)line, line_size);
      output_c << "\t/*" << hex << setw(6) << setfill('0') << written << "*/ ";
      for (auto c = 0; c < input.gcount(); c++)
        output_c << "0x" << setw(2) << (uint32_t)line[c] << ", ";
      written += input.gcount();
      output_c << "\n";
    }
    output_c << "}; // " << symbol << "\n\n";
  }

  output_c << "}  // namespace hut::" << argv[1] << '\n';

  output_h << "}  // namespace hut::" << argv[1] << '\n';

  output_c.flush();
  output_h.flush();

  std::cout << "Generated " << argv[1] << " at " << output_path << " in "
            << duration<double, std::milli>(steady_clock::now() - start).count() << "ms." << std::endl;
}
