cmake_minimum_required(VERSION 3.10)
project(Proactor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -glldb -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(SOURCE_FILES
    source/ThreadAffinity.cpp
)

set(HEADER_FILES
    source/AdaptiveSleeper.h
    source/Proactor.h
    source/Proactorpartition.h
    source/ThreadAffinity.h
    source/Queue.h
)

add_library(proactor_lib ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(proactor_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source)

# Platform-specific libraries
if(APPLE)
    target_link_libraries(proactor_lib PRIVATE "-framework CoreFoundation")
elseif(UNIX)
    target_link_libraries(proactor_lib PRIVATE pthread)
endif()

find_package(Boost 1.51.0 CONFIG REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(double-conversion REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(double-conversion REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(GTest 1.10 REQUIRED)

enable_testing()
# Test support files
set(TEST_SUPPORT_FILES
    test/Accumulator.h
)

# Test executable
add_executable(tests
  test/Proactortest.cpp
  test/Performancetest.cpp
  test/Queuetest.cpp
  ${TEST_SUPPORT_FILES}
)

target_link_libraries(tests
  PRIVATE
    ${FOLLY_LIBRARIES}
    gflags
    glog::glog
    double-conversion::double-conversion
    fmt::fmt
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    GTest::GTest
    GTest::Main
    proactor_lib
)


include(GoogleTest)
gtest_discover_tests(tests)