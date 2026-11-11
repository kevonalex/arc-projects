// Gateway gRPC client demo: StartCall (with deadline + retry), then watch stats briefly, then EndCall.
// NOT compiled by default. UNTESTED in the authoring environment - VERIFY. See README.md.
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "gateway.grpc.pb.h"
#include "gateway_core.hpp"  // retry_backoff_ms / is_retryable

namespace pb = etherstack::gateway::v1;

int main(int argc, char** argv) {
  const std::string addr = argc > 1 ? argv[1] : "127.0.0.1:50061";
  auto stub = pb::Gateway::NewStub(grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));

  pb::StartCallRequest req;
  req.set_call_id("demo-1");
  req.set_talkgroup(100);
  req.set_caller("alice");
  for (unsigned attempt = 0; attempt < 5; ++attempt) {
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(500));  // ALWAYS set a deadline
    pb::StartCallResponse resp;
    grpc::Status st = stub->StartCall(&ctx, req, &resp);
    if (st.ok()) {
      std::cout << "StartCall ok: " << resp.message() << "\n";
      break;
    }
    const auto code = static_cast<gw::Code>(st.error_code());
    std::cout << "StartCall failed (" << st.error_code() << "): " << st.error_message() << "\n";
    if (!gw::is_retryable(code, /*idempotent=*/true)) return 1;  // StartCall is idempotent by design in the core
    std::this_thread::sleep_for(std::chrono::milliseconds(gw::retry_backoff_ms(attempt, 100, 2000)));
  }

  grpc::ClientContext sctx;
  sctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(3));
  pb::WatchStatsRequest wr;
  wr.set_call_id("demo-1");
  wr.set_interval_ms(500);
  auto reader = stub->WatchStats(&sctx, wr);
  pb::CallStats s;
  while (reader->Read(&s)) std::cout << "stats: packets=" << s.packets() << " lost=" << s.lost() << " ber=" << s.ber() << " jitter_ms=" << s.jitter_ms() << "\n";
  std::cout << "WatchStats finished: " << reader->Finish().error_message() << "\n";

  grpc::ClientContext ectx;
  pb::EndCallRequest er;
  er.set_call_id("demo-1");
  pb::EndCallResponse eresp;
  std::cout << "EndCall: " << stub->EndCall(&ectx, er, &eresp).error_code() << "\n";
}
