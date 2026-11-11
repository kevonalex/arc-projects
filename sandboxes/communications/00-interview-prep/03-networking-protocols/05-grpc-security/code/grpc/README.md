# Optional: build the real gRPC CallControl server/client

The graded part of this pack (`tests/`) is **pure C++17**: you implement the *bytes* (varint, protobuf fields,
gRPC framing, HTTP/2 headers, deadlines, SRTP index/replay) and the **service logic** (`CallControlService`).
The files here wrap that logic with the real gRPC library. They are **skeleton quality and were NOT compiled in the
authoring environment** — treat compile errors as expected on first try and fix package names for your distro.

## 1. Install
* Ubuntu/WSL: `sudo apt install libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc cmake g++`
* macOS: `brew install grpc protobuf`
* vcpkg: `vcpkg install grpc protobuf`

## 2. Generate code by hand (what CMake automates)
```bash
cd code/grpc
protoc -I . --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc="$(which grpc_cpp_plugin)" callcontrol.proto
# produces callcontrol.pb.{h,cc} (messages) and callcontrol.grpc.pb.{h,cc} (Service base class + Stub)
g++ -std=c++17 -I. -I../solutions grpc_server.cpp callcontrol.pb.cc callcontrol.grpc.pb.cc \
    $(pkg-config --cflags --libs grpc++ protobuf) -lpthread -o callcontrol_server
```

## 3. Or via CMake
```bash
cmake -S .. -B build -DWITH_GRPC=ON          # add -DGRPC_USE_STUDENT=ON to wrap YOUR include/grpc_security.hpp
cmake --build build
./build/grpc/callcontrol_server &            # listens on 127.0.0.1:50051
./build/grpc/callcontrol_client              # StartCall -> WatchStats x5 -> cancel -> EndCall
```

## 4. What to observe (worksheet Part E)
* Wireshark on `lo`, filter `tcp.port == 50051`, *Analyze → Decode As… → HTTP2* (plaintext h2c only; with TLS you see ciphertext).
* Look for: connection preface `PRI * HTTP/2.0`, `SETTINGS`, `HEADERS` (`:path = /etherstack.callcontrol.v1.CallControl/StartCall`, `content-type: application/grpc`, `grpc-timeout: 2S`), `DATA` frames starting with the 5-byte gRPC prefix, and trailing `HEADERS` carrying `grpc-status`.
* Kill the client mid-stream: server `IsCancelled()` becomes true; RST_STREAM(CANCEL) appears on the wire.

## 5. Production notes
`InsecureServerCredentials` is for the lab only. Use TLS (`grpc::SslServerCredentials`) and, between gateway components, **mutual TLS**; set a deadline on every call; set `max_receive_message_size`; add keepalive pings for long streams.
