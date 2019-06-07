cmake_minimum_required(VERSION 3.8)

project(GrpcRateLimitPlugin)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(ENABLE_TESTING "Build the tests" OFF)

file(GLOB LIB_SOURCE_FILES "src/*.cpp")
add_library(${CMAKE_PROJECT_NAME} STATIC ${LIB_SOURCE_FILES})

set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
target_compile_options(
  ${CMAKE_PROJECT_NAME}
  PUBLIC
  -Wall
  -Werror
  -Wextra
  -Wpedantic
  -pedantic-errors
)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include(ExternalProject)
# Build grpc as an external project.
set(GRPC_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/grpc)
set(GRPC_INCLUDE_DIR ${GRPC_INSTALL_DIR}/include)
set(GRPC_LIB_DIR ${GRPC_INSTALL_DIR}/lib)
set(GRPC_EXTERNAL_LIB_DIR ${GRPC_INSTALL_DIR}/src/grpc_external_project-build/third_party)
set(GRPC_CPP_PLUGIN ${GRPC_INSTALL_DIR}/src/grpc_external_project-build/grpc_cpp_plugin)
ExternalProject_Add(
  grpc_external_project
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/grpc
  PREFIX ${GRPC_INSTALL_DIR}
  INSTALL_DIR ${GRPC_INSTALL_DIR}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${GRPC_INSTALL_DIR}
    -DgRPC_BUILD_TESTS=OFF
)
link_directories(${GRPC_LIB_DIR})
link_directories(${GRPC_INSTALL_DIR}/src/grpc_external_project-build)
link_directories(
  ${GRPC_EXTERNAL_LIB_DIR}/boringssl/ssl
  ${GRPC_EXTERNAL_LIB_DIR}/boringssl/crypto
  ${GRPC_EXTERNAL_LIB_DIR}/gflags
  ${GRPC_EXTERNAL_LIB_DIR}/zlib
)

# A target to combine all of the external projects.
add_custom_target(
  build_external_projects
  DEPENDS
    grpc_external_project
)

macro(add_base_dependencies target_name)
  add_dependencies(${target_name} build_external_projects)
  target_link_libraries(${target_name}
    grpc++
    grpc
    gpr
    protobuf
    gflags_nothreads
    cares
    z
    ssl
    crypto
    address_sorting
    Threads::Threads
  )
endmacro()

add_base_dependencies(${CMAKE_PROJECT_NAME})

target_include_directories(
  ${CMAKE_PROJECT_NAME}
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>"
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_include_directories(
  ${CMAKE_PROJECT_NAME}
  SYSTEM PUBLIC
    ${PROJECT_SOURCE_DIR}/third_party/grpc
    ${GRPC_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party/grpc/third_party/abseil-cpp
)

if(ENABLE_TESTING)
  enable_testing()
  add_subdirectory(test)
  add_custom_target(
    check
    COMMAND ${CMAKE_CTEST_COMMAND}
    --force-new-ctest-process
    --verbose)
endif()
