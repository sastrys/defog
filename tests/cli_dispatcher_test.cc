#include "cli/cli.h"

#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace {

using defog::cli::CommandResult;
using defog::cli::Dispatch;

CommandResult Run(std::initializer_list<std::string_view> args) {
  return Dispatch(std::vector<std::string_view>(args));
}

TEST(CliDispatcherTest, GlobalHelpListsSupportedCommands) {
  const CommandResult result = Run({"--help"});

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_NE(result.stdout_text.find("Usage:"), std::string::npos);
  for (std::string_view command : {"update", "status", "changed", "ls", "find",
                                  "show", "graph", "inspect", "report"}) {
    EXPECT_NE(result.stdout_text.find(command), std::string::npos) << command;
  }
  EXPECT_TRUE(result.stderr_text.empty());
}

TEST(CliDispatcherTest, RoutesEverySupportedCommand) {
  const std::vector<std::vector<std::string_view>> commands = {
      {"update"},       {"status"},       {"changed"},
      {"ls"},           {"find", "query"}, {"show"},
      {"graph", "calls"}, {"inspect"},      {"report"},
  };

  for (const auto& command : commands) {
    const CommandResult result = Dispatch(command);
    EXPECT_EQ(result.exit_code, 0) << command.front();
    if (command.front() == "status") {
      EXPECT_NE(result.stdout_text.find("Defog status"), std::string::npos)
          << result.stdout_text;
    } else {
      EXPECT_NE(result.stdout_text.find(std::string("dfog ") + std::string(command.front())),
                std::string::npos)
          << result.stdout_text;
    }
    EXPECT_TRUE(result.stderr_text.empty()) << result.stderr_text;
  }
}


TEST(CliDispatcherTest, ParsesCommandOptionsBeforeDispatch) {
  const CommandResult update = Run({"update", "--force"});
  EXPECT_EQ(update.exit_code, 0);
  EXPECT_NE(update.stdout_text.find("force"), std::string::npos);
  EXPECT_TRUE(update.stderr_text.empty());

  const CommandResult changed = Run({"changed", "--kind=function", "-q"});
  EXPECT_EQ(changed.exit_code, 0);
  EXPECT_NE(changed.stdout_text.find("kind: function"), std::string::npos);
  EXPECT_NE(changed.stdout_text.find("quiet"), std::string::npos);
  EXPECT_TRUE(changed.stderr_text.empty());
}

TEST(CliDispatcherTest, MissingGraphOutputValueReturnsHelpfulError) {
  const CommandResult result = Run({"graph", "-o"});

  EXPECT_EQ(result.exit_code, 2);
  EXPECT_TRUE(result.stdout_text.empty());
  EXPECT_NE(result.stderr_text.find("missing value for -o"), std::string::npos);
}

TEST(CliDispatcherTest, UnknownCommandReturnsHelpfulError) {
  const CommandResult result = Run({"frobnicate"});

  EXPECT_EQ(result.exit_code, 2);
  EXPECT_TRUE(result.stdout_text.empty());
  EXPECT_NE(result.stderr_text.find("unknown command 'frobnicate'"), std::string::npos);
  EXPECT_NE(result.stderr_text.find("dfog --help"), std::string::npos);
}

TEST(CliDispatcherTest, MissingRequiredFindQueryReturnsHelpfulError) {
  const CommandResult result = Run({"find"});

  EXPECT_EQ(result.exit_code, 2);
  EXPECT_TRUE(result.stdout_text.empty());
  EXPECT_NE(result.stderr_text.find("missing required query"), std::string::npos);
}

TEST(CliDispatcherTest, MissingKindValueReturnsHelpfulError) {
  const CommandResult result = Run({"changed", "--kind"});

  EXPECT_EQ(result.exit_code, 2);
  EXPECT_TRUE(result.stdout_text.empty());
  EXPECT_NE(result.stderr_text.find("missing value for --kind"), std::string::npos);
}

TEST(CliDispatcherTest, ConflictingOutputModesReturnHelpfulError) {
  const CommandResult result = Run({"changed", "-q", "--jsonl"});

  EXPECT_EQ(result.exit_code, 2);
  EXPECT_TRUE(result.stdout_text.empty());
  EXPECT_NE(result.stderr_text.find("choose only one output mode"), std::string::npos);
}

TEST(CliDispatcherTest, CommandHelpIsCommandSpecific) {
  const CommandResult result = Run({"graph", "--help"});

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_NE(result.stdout_text.find("Usage: dfog graph"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("calls|deps"), std::string::npos);
  EXPECT_TRUE(result.stderr_text.empty());
}

}  // namespace
