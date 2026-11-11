# Optional gRPC build (configure the parent with -DWITH_GRPC=ON). Needs protobuf + gRPC with CMake config files:
#   Ubuntu/WSL:  sudo apt install libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc
#   vcpkg:       vcpkg install grpc protobuf    +  -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake
# UNTESTED in the authoring environment (no gRPC installed): VERIFY package/target names for your distro (see README.md).
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
  find_package(Protobuf REQUIRED)
endif()
find_package(gRPC CONFIG REQUIRED)

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/gateway.proto)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(GEN_SRCS ${GEN_DIR}/gateway.pb.cc ${GEN_DIR}/gateway.grpc.pb.cc)
add_custom_command(
  OUTPUT ${GEN_SRCS} ${GEN_DIR}/gateway.pb.h ${GEN_DIR}/gateway.grpc.pb.h
  COMMAND $<TARGET_FILE:protobuf::protoc>
  ARGS --cpp_out=${GEN_DIR} --grpc_out=${GEN_DIR}
       --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
       -I ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILE}
  DEPENDS ${PROTO_FILE})
add_library(gateway_proto STATIC ${GEN_SRCS})
target_include_directories(gateway_proto PUBLIC ${GEN_DIR})
target_link_libraries(gateway_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)

set(CORE_INC ${CMAKE_CURRENT_SOURCE_DIR}/../solutions)
if(GRPC_USE_STUDENT)
  set(CORE_INC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
endif()
foreach(t gateway_server gateway_client)
  add_executable(${t} ${t}.cpp)
  target_include_directories(${t} PRIVATE ${CORE_INC})
  target_link_libraries(${t} PRIVATE gateway_proto Threads::Threads)
endforeach()
