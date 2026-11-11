# Optional gRPC wrapper (Capstone M4)

**Status: skeleton, UNTESTED.** The authoring environment had no gRPC/protobuf installed, so these files were never compiled.
Treat them as a reading/porting exercise; **VERIFY** package and target names on your machine.

The unit tests (`../tests/test_gateway.cpp`) never touch gRPC. That is the point: the adapter is thin enough to read in five minutes.

| RPC | Shape | Core call | Error mapping |
|---|---|---|---|
| `StartCall` | unary | `GatewayService::start_call` | `gw::Code` numbers equal `grpc::StatusCode`, so `static_cast` |
| `EndCall` | unary | `end_call` | `NotFound` if absent |
| `WatchStats` | server streaming | `snapshot` in a loop | ends on cancel / deadline / call gone |
| `FloorChannel` | bidirectional | `floor_request`, `floor_release`, `floor_events` | SKELETON: no timer thread / multi-subscriber fan-out |

## Build

```bash
sudo apt install libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc    # Ubuntu / WSL (names: VERIFY)
cmake -S .. -B ../build-grpc -DWITH_GRPC=ON            # add -DGRPC_USE_STUDENT=ON to link YOUR include/gateway_core.hpp
cmake --build ../build-grpc --target gateway_server gateway_client
../build-grpc/grpc/gateway_server &     # listens on 127.0.0.1:50061
../build-grpc/grpc/gateway_client
```

## Things to notice (interview material)

* **Threading:** gRPC calls handlers from several threads; the core is deliberately single-threaded; the adapter holds one mutex. Why is that the right split?
* **Deadlines:** the client sets one on every call. Without it a dead server hangs the caller forever.
* **Idempotency:** `StartCall` repeated with identical arguments is `Ok`, so the client may retry it after `UNAVAILABLE` or `DEADLINE_EXCEEDED`.
* **Insecure credentials** are for localhost only. Beyond localhost use TLS (mTLS between services).
* **Exercise (W 5.x):** add a timer thread that calls `tick()` every 100 ms and fans events out to every `FloorChannel` stream of the call.

Data rule for the capstone: synthetic or recorded-with-permission audio only. No live public-safety reception.
