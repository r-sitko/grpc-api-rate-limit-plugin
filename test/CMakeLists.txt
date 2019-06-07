cmake_minimum_required(VERSION 3.8)

set(GTESTS_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/gtests)
set(GTESTS_INCLUDE_DIR ${GTESTS_INSTALL_DIR}/include)
set(GTESTS_LIB_DIR ${GTESTS_INSTALL_DIR}/lib)
ExternalProject_Add(
  gtests_external_project
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/googletest
  PREFIX ${GTESTS_INSTALL_DIR}
  INSTALL_DIR ${GTESTS_INSTALL_DIR}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${GTESTS_INSTALL_DIR}
    -DBUILD_GMOCK=OFF
)
link_directories(${GTESTS_LIB_DIR})

# A target to aggregate all of the external projects.
add_custom_target(
  test_build_external_projects
  DEPENDS
    gtests_external_project
)

macro(package_add_test TESTNAME)
  add_executable(${TESTNAME} ${ARGN})
  add_dependencies(${TESTNAME} test_build_external_projects)
  target_include_directories(
    ${TESTNAME}
    SYSTEM PRIVATE
    ${GTESTS_INCLUDE_DIR}
  )
  target_link_libraries(
    ${TESTNAME}
    ${CMAKE_PROJECT_NAME}
    gtest
    gtest_main
  )
  add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
  set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)

  set_target_properties(
    ${TESTNAME}
    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
  )
  target_compile_options(
    ${TESTNAME}
    PUBLIC
    -Wall
    -Werror
    -Wextra
    -Wpedantic
    -pedantic-errors
  )
endmacro()

package_add_test(
  ${CMAKE_PROJECT_NAME}Test
  "src/RateLimiterTest.cpp"
  "src/GrpcRateLimitEnd2EndTest.cpp"
)

set(PROTOC ${GRPC_INSTALL_DIR}/bin/protoc)
set(PROTOBUF_INCLUDE_DIR ${GRPC_INSTALL_DIR}/include)

macro(make_protobuf_cpp_lib LIB_NAME HDRS_OUT USE_GRPC)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/protos")
  set(_protofiles)
  set(_generated_srcs)
  set(_generated_hdrs)

  foreach(name ${ARGN})
      list(APPEND _protofiles "${PROJECT_SOURCE_DIR}/test/protos/${name}.proto")
      list(APPEND _generated_srcs "${CMAKE_BINARY_DIR}/protos/${name}.pb.cc")
      list(APPEND _generated_hdrs "${CMAKE_BINARY_DIR}/protos/${name}.pb.h")
      set_source_files_properties("${CMAKE_BINARY_DIR}/protos/${name}.pb.cc" PROPERTIES GENERATED TRUE)
      set_source_files_properties("${CMAKE_BINARY_DIR}/protos/${name}.pb.h" PROPERTIES GENERATED TRUE)
      if(${USE_GRPC})
        list(APPEND _generated_srcs "${CMAKE_BINARY_DIR}/protos/${name}.grpc.pb.cc")
        list(APPEND _generated_hdrs "${CMAKE_BINARY_DIR}/protos/${name}.grpc.pb.h")
        set_source_files_properties("${CMAKE_BINARY_DIR}/protos/${name}.grpc.pb.cc" PROPERTIES GENERATED TRUE)
        set_source_files_properties("${CMAKE_BINARY_DIR}/protos/${name}.grpc.pb.h" PROPERTIES GENERATED TRUE)
      endif()
  endforeach()
  set(_grpc_spec)
  if (${USE_GRPC})
    set(_grpc_spec
      --grpc_out=${CMAKE_BINARY_DIR}/protos
      --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
    )
  endif()

  add_custom_command(OUTPUT ${_generated_srcs} ${_generated_hdrs}
    COMMAND ${PROTOC} ${_protofiles}
            -I ${PROJECT_SOURCE_DIR}/test/protos
            -I ${PROTOBUF_INCLUDE_DIR}
            --cpp_out=${CMAKE_BINARY_DIR}/protos
            ${_grpc_spec}
    DEPENDS ${_protofiles}
    VERBATIM
  )

  add_library(
    ${LIB_NAME}
    ${_generated_srcs}
  )
  target_include_directories(
    ${LIB_NAME}
    SYSTEM PUBLIC
      ${CMAKE_BINARY_DIR}/protos
      ${PROTOBUF_INCLUDE_DIR}
  )

  set(${HDRS_OUT} ${_generated_hdrs})
endmacro()

set_source_files_properties(${PROTO_HDRS} PROPERTIES GENERATED TRUE)
make_protobuf_cpp_lib(
  proto_files
  PROTO_HDRS
  true
  "pingpong"
)

target_link_libraries(${CMAKE_PROJECT_NAME}Test proto_files ${CMAKE_PROJECT_NAME})
