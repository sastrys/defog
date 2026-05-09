#include "cli/cli.h"

#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

int main(int argc, char* argv[]) {
  std::vector<std::string_view> args;
  args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0);
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  return defog::cli::Run(std::move(args), std::cout, std::cerr);
}
