include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  TLS_VERIFY OFF
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_FILES "*.cpp")

add_executable(devfinder_tests ${TEST_FILES})
target_link_libraries(devfinder_tests PRIVATE devfinder_core gtest_main)

include(GoogleTest)
gtest_discover_tests(devfinder_tests)
