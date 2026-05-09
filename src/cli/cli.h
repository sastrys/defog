#ifndef DEFOG_SRC_CLI_CLI_H_
#define DEFOG_SRC_CLI_CLI_H_

#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace defog::cli {

struct CommandResult {
  int exit_code = 0;
  std::string stdout_text;
  std::string stderr_text;
};

CommandResult Dispatch(std::vector<std::string_view> args);
int Run(std::vector<std::string_view> args, std::ostream& out, std::ostream& err);

}  // namespace defog::cli

#endif  // DEFOG_SRC_CLI_CLI_H_
