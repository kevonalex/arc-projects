// CallControl gRPC client: StartCall -> WatchStats (5 updates, then cancel) -> EndCall, each with a DEADLINE.
// NOT compiled by default. See grpc/README.md. UNTESTED in the authoring environment - VERIFY.
#include <chrono>
#include <iostream>
#include <memory>

#include <grpcpp/grpcpp.h>
#include "callcontrol.grpc.pb.h"

namespace pb = etherstack::callcontrol::v1;
using Clock = std::chrono::system_clock;

int main(int argc, char** argv) {
    std::string addr = argc > 1 ? argv[1] : "127.0.0.1:50051";
    auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());   // DEV ONLY
    std::unique_ptr<pb::CallControl::Stub> stub = pb::CallControl::NewStub(channel);

    {   // 1) unary with a 2 s deadline - ALWAYS set one; the default is "wait forever"
        grpc::ClientContext ctx;
        ctx.set_deadline(Clock::now() + std::chrono::seconds(2));
        pb::StartCallRequest req; req.set_call_id("demo"); req.set_talkgroup(7); req.set_caller("radio-7");
        pb::StartCallResponse resp;
        grpc::Status st = stub->StartCall(&ctx, req, &resp);
        std::cout << "StartCall: " << st.error_code() << " " << st.error_message() << " accepted=" << resp.accepted() << "\n";
    }
    {   // 2) server streaming: read until we have enough, then cancel (the server sees ctx->IsCancelled())
        grpc::ClientContext ctx;
        ctx.set_deadline(Clock::now() + std::chrono::seconds(30));
        pb::WatchStatsRequest req; req.set_call_id("demo"); req.set_interval_ms(200);
        auto reader = stub->WatchStats(&ctx, req);
        pb::CallStats s; int n = 0;
        while (n < 5 && reader->Read(&s)) { std::cout << "stats pkts=" << s.packets() << " lost=" << s.lost() << " jitter=" << s.jitter_ms() << "\n"; ++n; }
        ctx.TryCancel();
        grpc::Status st = reader->Finish();                           // CANCELLED expected
        std::cout << "WatchStats finished: " << st.error_code() << "\n";
    }
    {   // 3) unary EndCall
        grpc::ClientContext ctx;
        ctx.set_deadline(Clock::now() + std::chrono::seconds(2));
        pb::EndCallRequest req; req.set_call_id("demo");
        pb::EndCallResponse resp;
        grpc::Status st = stub->EndCall(&ctx, req, &resp);
        std::cout << "EndCall: " << st.error_code() << " ok=" << resp.ok() << "\n";
    }
}
