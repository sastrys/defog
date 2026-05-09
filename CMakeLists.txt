cmake_minimum_required(VERSION 3.24)

project(defog
  VERSION 0.1.0
  DESCRIPTION "Defog human-first code comprehension tool"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(defog_cli
  src/cli/cli.cc)
add_library(defog::cli ALIAS defog_cli)
target_include_directories(defog_cli
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src)
target_compile_features(defog_cli PUBLIC cxx_std_20)

add_executable(dfog
  src/cli/main.cc)
target_link_libraries(dfog PRIVATE defog::cli)

include(CTest)
if(BUILD_TESTING)
  option(DFOG_FETCH_GOOGLETEST "Download GoogleTest when no installed package is available" OFF)

  find_package(GTest QUIET)
  set(DFOG_HAS_REAL_GTEST FALSE)
  if(GTest_FOUND)
    set(DFOG_TEST_MAIN_TARGET GTest::gtest_main)
    set(DFOG_HAS_REAL_GTEST TRUE)
  elseif(DFOG_FETCH_GOOGLETEST)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    set(DFOG_TEST_MAIN_TARGET GTest::gtest_main)
    set(DFOG_HAS_REAL_GTEST TRUE)
  else()
    add_library(dfog_gtest_compat_main
      tests/gtest_compat/gtest_main.cc)
    target_include_directories(dfog_gtest_compat_main
      PUBLIC
        ${PROJECT_SOURCE_DIR}/tests/gtest_compat)
    add_library(GTest::gtest_main ALIAS dfog_gtest_compat_main)
    set(DFOG_TEST_MAIN_TARGET GTest::gtest_main)
  endif()

  add_executable(dfog_cli_test
    tests/cli_dispatcher_test.cc)
  target_link_libraries(dfog_cli_test
    PRIVATE
      defog::cli
      ${DFOG_TEST_MAIN_TARGET})

  if(DFOG_HAS_REAL_GTEST)
    include(GoogleTest)
    gtest_discover_tests(dfog_cli_test)
  else()
    add_test(NAME dfog_cli_test COMMAND dfog_cli_test)
  endif()
endif()
