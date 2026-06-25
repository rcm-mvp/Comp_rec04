cmake_minimum_required(VERSION 3.24)

project(im110_bufferpool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(IM110_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
set(IM110_SANITIZER "" CACHE STRING "Sanitizer to enable: address, undefined, thread")
set_property(CACHE IM110_SANITIZER PROPERTY STRINGS "" address undefined thread)

include(FetchContent)

FetchContent_Declare(
  fmt
  URL https://github.com/fmtlib/fmt/archive/refs/tags/10.2.1.zip
)
FetchContent_Declare(
  cli11
  URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.4.2.zip
)
FetchContent_Declare(
  catch2
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.4.zip
)

FetchContent_MakeAvailable(fmt cli11 catch2)

add_library(
  im110_bufferpool
  src/buffer/arc_replacer.cpp
  src/buffer/buffer_pool_manager.cpp
  src/storage/disk_scheduler.cpp
  src/storage/page_allocator.cpp
  src/storage/page_guard.cpp
  src/storage/page_store.cpp
)

target_include_directories(im110_bufferpool PUBLIC include)
target_link_libraries(im110_bufferpool PUBLIC fmt::fmt)

target_compile_features(im110_bufferpool PUBLIC cxx_std_20)
target_compile_options(
  im110_bufferpool
  PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic -Wconversion -Wshadow>
    $<$<AND:$<BOOL:${IM110_WARNINGS_AS_ERRORS}>,$<CXX_COMPILER_ID:AppleClang,Clang,GNU>>:-Werror>
)

function(im110_apply_sanitizer target)
  if(IM110_SANITIZER STREQUAL "address")
    target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address)
  elseif(IM110_SANITIZER STREQUAL "undefined")
    target_compile_options(${target} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=undefined)
  elseif(IM110_SANITIZER STREQUAL "thread")
    target_compile_options(${target} PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=thread)
  elseif(NOT IM110_SANITIZER STREQUAL "")
    message(FATAL_ERROR "Unknown sanitizer: ${IM110_SANITIZER}")
  endif()
endfunction()

im110_apply_sanitizer(im110_bufferpool)

add_executable(bufferpool_demo tools/bufferpool_demo.cpp)
target_link_libraries(bufferpool_demo PRIVATE im110_bufferpool CLI11::CLI11 fmt::fmt)
im110_apply_sanitizer(bufferpool_demo)

enable_testing()

add_executable(infrastructure_tests tests/infrastructure_test.cpp)
target_link_libraries(infrastructure_tests PRIVATE im110_bufferpool Catch2::Catch2WithMain)
im110_apply_sanitizer(infrastructure_tests)

add_executable(
  assignment_tests
  tests/arc_replacer_test.cpp
  tests/disk_scheduler_test.cpp
  tests/page_guard_test.cpp
  tests/buffer_pool_manager_test.cpp
  tests/dbms_workload_test.cpp
)
target_link_libraries(assignment_tests PRIVATE im110_bufferpool Catch2::Catch2WithMain)
im110_apply_sanitizer(assignment_tests)

add_executable(disk_scheduler_perf_test tests/disk_scheduler_perf_test.cpp)
target_link_libraries(disk_scheduler_perf_test PRIVATE im110_bufferpool Catch2::Catch2WithMain)
im110_apply_sanitizer(disk_scheduler_perf_test)

include(CTest)
add_test(NAME infrastructure_tests COMMAND infrastructure_tests)
set_tests_properties(infrastructure_tests PROPERTIES LABELS "infra")

add_test(NAME assignment_tests COMMAND assignment_tests)
set_tests_properties(assignment_tests PROPERTIES LABELS "assignment")

# Perf test is opt-in: invoke with `ctest -L perf`. Excluded from the default
# run so plain `ctest` stays fast.
add_test(NAME disk_scheduler_perf_test COMMAND disk_scheduler_perf_test)
set_tests_properties(disk_scheduler_perf_test PROPERTIES LABELS "perf")
