#ifndef DEFOG_TESTS_GTEST_COMPAT_GTEST_GTEST_H_
#define DEFOG_TESTS_GTEST_COMPAT_GTEST_GTEST_H_

#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace testing {
namespace internal {

struct TestCase {
  std::string suite;
  std::string name;
  std::function<void()> body;
};

inline std::vector<TestCase>& Registry() {
  static std::vector<TestCase>* registry = new std::vector<TestCase>();
  return *registry;
}

inline int& FailureCount() {
  static int failures = 0;
  return failures;
}

inline void AddFailure(const char* file, int line, std::string_view expression,
                       const std::string& details = {}) {
  ++FailureCount();
  std::cerr << file << ':' << line << ": failure: " << expression;
  if (!details.empty()) std::cerr << " (" << details << ')';
  std::cerr << '\n';
}

class Expectation {
 public:
  template <typename T>
  Expectation& operator<<(const T&) {
    return *this;
  }
};

class TestRegistrar {
 public:
  TestRegistrar(const char* suite, const char* name, std::function<void()> body) {
    Registry().push_back(TestCase{suite, name, std::move(body)});
  }
};

template <typename Left, typename Right>
Expectation ExpectEq(const Left& left, const Right& right, const char* left_expr,
                     const char* right_expr, const char* file, int line) {
  if (!(left == right)) {
    std::ostringstream details;
    details << left_expr << " vs " << right_expr;
    AddFailure(file, line, "EXPECT_EQ", details.str());
  }
  return {};
}

template <typename Left, typename Right>
Expectation ExpectNe(const Left& left, const Right& right, const char* left_expr,
                     const char* right_expr, const char* file, int line) {
  if (!(left != right)) {
    std::ostringstream details;
    details << left_expr << " vs " << right_expr;
    AddFailure(file, line, "EXPECT_NE", details.str());
  }
  return {};
}

inline Expectation ExpectTrue(bool value, const char* expr, const char* file,
                              int line) {
  if (!value) AddFailure(file, line, expr);
  return {};
}

inline int RunAllTests(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--gtest_list_tests") {
      std::string current_suite;
      for (const TestCase& test : Registry()) {
        if (test.suite != current_suite) {
          current_suite = test.suite;
          std::cout << current_suite << ".\n";
        }
        std::cout << "  " << test.name << "\n";
      }
      return 0;
    }
  }

  for (const TestCase& test : Registry()) {
    const int before = FailureCount();
    test.body();
    if (FailureCount() == before) {
      std::cout << "[  PASSED  ] " << test.suite << '.' << test.name << '\n';
    } else {
      std::cout << "[  FAILED  ] " << test.suite << '.' << test.name << '\n';
    }
  }
  return FailureCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace internal
}  // namespace testing

#define TEST(suite_name, test_name)                                             \
  void suite_name##_##test_name##_Test();                                       \
  static ::testing::internal::TestRegistrar                                     \
      suite_name##_##test_name##_registrar(#suite_name, #test_name,             \
                                           suite_name##_##test_name##_Test);     \
  void suite_name##_##test_name##_Test()

#define EXPECT_EQ(left, right)                                                  \
  ::testing::internal::ExpectEq((left), (right), #left, #right, __FILE__,        \
                                __LINE__)
#define EXPECT_NE(left, right)                                                  \
  ::testing::internal::ExpectNe((left), (right), #left, #right, __FILE__,        \
                                __LINE__)
#define EXPECT_TRUE(expr)                                                       \
  ::testing::internal::ExpectTrue(static_cast<bool>(expr), #expr, __FILE__,      \
                                  __LINE__)

#endif  // DEFOG_TESTS_GTEST_COMPAT_GTEST_GTEST_H_
