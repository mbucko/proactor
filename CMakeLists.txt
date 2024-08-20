cmake_minimum_required(VERSION 3.10)
project(Proactor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -glldb -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(SOURCE_FILES
    source/proactor.cpp
    source/ThreadAffinity.cpp
)

set(HEADER_FILES
    source/proactor.h
    source/proactorpartition.h
    source/lockfreequeue.h
    source/ThreadAffinity.h
)

add_library(proactor_lib ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(proactor_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source)

# Platform-specific libraries
if(APPLE)
    target_link_libraries(proactor_lib PRIVATE "-framework CoreFoundation")
elseif(UNIX)
    target_link_libraries(proactor_lib PRIVATE pthread)
endif()

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
# Test support files
set(TEST_SUPPORT_FILES
    test/accumulator.cpp
    test/accumulator.h
)

# Test executable
add_executable(tests
  test/proactortest.cpp
  test/queuetest.cpp
  ${TEST_SUPPORT_FILES}
)
target_link_libraries(tests
  PRIVATE
    proactor_lib
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(tests)