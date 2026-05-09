#include "cli/cli.h"

#include <algorithm>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace defog::cli {
namespace {

struct CommandSpec {
  std::string_view name;
  std::string_view summary;
};

struct ParsedArguments {
  bool wants_help = false;
  bool force = false;
  bool quiet = false;
  bool jsonl = false;
  bool verbose = false;
  bool changed = false;
  bool open = false;
  std::string kind;
  std::string output_path;
  std::vector<std::string_view> positionals;
};

struct ParseResult {
  ParsedArguments args;
  std::optional<CommandResult> error;
};

constexpr CommandSpec kCommands[] = {
    {"update", "refresh the local Defog index"},
    {"status", "show index and Git change status"},
    {"changed", "list changed targets"},
    {"ls", "list indexed targets"},
    {"find", "find indexed targets"},
    {"show", "show target source and summary"},
    {"graph", "emit a target graph"},
    {"inspect", "inspect target metadata"},
    {"report", "generate a local report"},
};

bool IsHelp(std::string_view arg) { return arg == "--help" || arg == "-h"; }

bool IsSelector(std::string_view command) {
  return command == "changed" || command == "ls" || command == "find";
}

bool IsKnownCommand(std::string_view command) {
  return std::any_of(std::begin(kCommands), std::end(kCommands),
                     [command](const CommandSpec& spec) {
                       return spec.name == command;
                     });
}

std::string GlobalHelp() {
  std::string text =
      "Defog (dfog) - human-first code comprehension\n\n"
      "Usage:\n"
      "  dfog <command> [options] [args]\n\n"
      "Commands:\n";
  for (const auto& command : kCommands) {
    text += "  ";
    text += command.name;
    text += std::string(10 - std::min<std::size_t>(command.name.size(), 10), ' ');
    text += command.summary;
    text += "\n";
  }
  text +=
      "\nRun 'dfog <command> --help' for command-specific help.\n";
  return text;
}

std::string CommandHelp(std::string_view command) {
  if (command == "update") {
    return "Usage: dfog update [--force]\n\nRefresh the local Defog index.\n";
  }
  if (command == "status") {
    return "Usage: dfog status\n\nShow index and Git change status.\n";
  }
  if (command == "changed") {
    return "Usage: dfog changed [--kind <kind>] [-q|--jsonl]\n\nList changed targets.\n";
  }
  if (command == "ls") {
    return "Usage: dfog ls [path] [--kind <kind>] [-q|--jsonl]\n\nList indexed targets.\n";
  }
  if (command == "find") {
    return "Usage: dfog find <query> [--kind <kind>] [-q|--jsonl]\n\nFind indexed targets.\n";
  }
  if (command == "show") {
    return "Usage: dfog show [-v] [target-ref ...]\n\nShow target source and summary.\n";
  }
  if (command == "graph") {
    return "Usage: dfog graph [calls|deps] [target-ref ...] [-o <path>]\n\nEmit a target graph.\n";
  }
  if (command == "inspect") {
    return "Usage: dfog inspect [target-ref ...]\n\nInspect target metadata.\n";
  }
  if (command == "report") {
    return "Usage: dfog report [--changed] [--open]\n\nGenerate a local report.\n";
  }
  return GlobalHelp();
}

CommandResult Error(std::string message) {
  message += "\nRun 'dfog --help' for usage.\n";
  return CommandResult{2, "", message};
}

std::optional<std::string_view> ReadOptionValue(
    std::string_view command, std::string_view option,
    const std::vector<std::string_view>& raw_args, std::size_t* index,
    std::optional<CommandResult>* error) {
  if (*index + 1 >= raw_args.size() || raw_args[*index + 1].starts_with("-")) {
    *error = Error("dfog " + std::string(command) + ": missing value for " +
                   std::string(option));
    return std::nullopt;
  }
  ++(*index);
  return raw_args[*index];
}

ParseResult ParseArguments(std::string_view command,
                           const std::vector<std::string_view>& raw_args) {
  ParsedArguments parsed;

  for (std::size_t i = 1; i < raw_args.size(); ++i) {
    const std::string_view arg = raw_args[i];
    if (IsHelp(arg)) {
      parsed.wants_help = true;
      return ParseResult{std::move(parsed), std::nullopt};
    }

    if (command == "update" && arg == "--force") {
      parsed.force = true;
      continue;
    }
    if (IsSelector(command) && arg == "-q") {
      parsed.quiet = true;
      continue;
    }
    if (IsSelector(command) && arg == "--jsonl") {
      parsed.jsonl = true;
      continue;
    }
    if (IsSelector(command) && arg == "--kind") {
      std::optional<CommandResult> error;
      const std::optional<std::string_view> value =
          ReadOptionValue(command, "--kind", raw_args, &i, &error);
      if (error.has_value()) return ParseResult{std::move(parsed), error};
      parsed.kind = *value;
      continue;
    }
    if (IsSelector(command) && arg.starts_with("--kind=")) {
      parsed.kind = arg.substr(std::string_view("--kind=").size());
      if (parsed.kind.empty()) {
        return ParseResult{std::move(parsed),
                           Error("dfog " + std::string(command) +
                                 ": missing value for --kind")};
      }
      continue;
    }
    if (command == "show" && arg == "-v") {
      parsed.verbose = true;
      continue;
    }
    if (command == "report" && arg == "--changed") {
      parsed.changed = true;
      continue;
    }
    if (command == "report" && arg == "--open") {
      parsed.open = true;
      continue;
    }
    if (command == "graph" && arg == "-o") {
      std::optional<CommandResult> error;
      const std::optional<std::string_view> value =
          ReadOptionValue(command, "-o", raw_args, &i, &error);
      if (error.has_value()) return ParseResult{std::move(parsed), error};
      parsed.output_path = *value;
      continue;
    }
    if (arg.starts_with("-")) {
      return ParseResult{std::move(parsed),
                         Error("dfog " + std::string(command) +
                               ": unknown option '" + std::string(arg) + "'")};
    }

    parsed.positionals.push_back(arg);
  }

  return ParseResult{std::move(parsed), std::nullopt};
}

bool HasConflictingOutputModes(const ParsedArguments& args) {
  return args.quiet && args.jsonl;
}

CommandResult PlaceholderSuccess(std::string_view command, std::string detail) {
  std::string text = "dfog ";
  text += command;
  text += ": ";
  text += detail;
  text += "\n";
  return CommandResult{0, text, ""};
}

CommandResult DispatchUpdate(const ParsedArguments& args) {
  if (!args.positionals.empty()) {
    return Error("dfog update: unexpected argument '" +
                 std::string(args.positionals[0]) + "'");
  }
  return PlaceholderSuccess("update", args.force ? "index refresh requested (force)"
                                                 : "index refresh requested");
}

CommandResult DispatchNoArgumentCommand(std::string_view command,
                                        const ParsedArguments& args) {
  if (!args.positionals.empty()) {
    return Error("dfog " + std::string(command) + ": unexpected argument '" +
                 std::string(args.positionals[0]) + "'");
  }
  if (command == "status") {
    return CommandResult{0,
                         "Defog status\n\nIndex:\n  not built\n\nTry:\n  dfog update\n  dfog changed\n  dfog report --changed\n",
                         ""};
  }
  return PlaceholderSuccess(command, "ok");
}

CommandResult DispatchSelector(std::string_view command, const ParsedArguments& args) {
  if (HasConflictingOutputModes(args)) {
    return Error("dfog " + std::string(command) +
                 ": choose only one output mode: -q or --jsonl");
  }
  if (command == "changed" && !args.positionals.empty()) {
    return Error("dfog changed: unexpected argument '" +
                 std::string(args.positionals[0]) + "'");
  }
  if (command == "ls" && args.positionals.size() > 1) {
    return Error("dfog ls: expected at most one path argument");
  }
  if (command == "find" && args.positionals.empty()) {
    return Error("dfog find: missing required query");
  }
  if (command == "find" && args.positionals.size() > 1) {
    return Error("dfog find: expected exactly one query argument");
  }

  std::string detail = "no indexed targets yet";
  if (!args.kind.empty()) detail += " (kind: " + args.kind + ")";
  if (args.quiet) detail += " (quiet)";
  if (args.jsonl) detail += " (jsonl)";
  return PlaceholderSuccess(command, detail);
}

CommandResult DispatchConsumer(std::string_view command, const ParsedArguments& args) {
  if (command == "graph" && !args.positionals.empty() && args.positionals[0] != "calls" &&
      args.positionals[0] != "deps") {
    return Error("dfog graph: graph type must be 'calls' or 'deps'");
  }
  if (command == "report" && !args.positionals.empty()) {
    return Error("dfog report: unexpected argument '" +
                 std::string(args.positionals[0]) + "'");
  }

  std::string detail = "ready";
  if (args.verbose) detail += " (verbose)";
  if (args.changed) detail += " (changed)";
  if (args.open) detail += " (open)";
  if (!args.output_path.empty()) detail += " (output: " + args.output_path + ")";
  return PlaceholderSuccess(command, detail);
}

}  // namespace

CommandResult Dispatch(std::vector<std::string_view> args) {
  if (args.empty() || IsHelp(args[0])) return CommandResult{0, GlobalHelp(), ""};

  const std::string_view command = args[0];
  if (!IsKnownCommand(command)) {
    return Error("dfog: unknown command '" + std::string(command) + "'");
  }

  ParseResult parsed = ParseArguments(command, args);
  if (parsed.error.has_value()) return *parsed.error;
  if (parsed.args.wants_help) return CommandResult{0, CommandHelp(command), ""};

  if (command == "update") return DispatchUpdate(parsed.args);
  if (command == "status") return DispatchNoArgumentCommand(command, parsed.args);
  if (IsSelector(command)) return DispatchSelector(command, parsed.args);
  return DispatchConsumer(command, parsed.args);
}

int Run(std::vector<std::string_view> args, std::ostream& out, std::ostream& err) {
  const CommandResult result = Dispatch(std::move(args));
  out << result.stdout_text;
  err << result.stderr_text;
  return result.exit_code;
}

}  // namespace defog::cli
