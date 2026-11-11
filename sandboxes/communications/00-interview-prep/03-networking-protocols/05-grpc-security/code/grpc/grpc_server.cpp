// CallControl gRPC server: a THIN adapter over ether::CallControlService (the logic you unit-tested).
// NOT compiled by default (needs gRPC + protobuf). See grpc/README.md. UNTESTED in the authoring environment - VERIFY.
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

#include <grpcpp/grpcpp.h>
#include "callcontrol.grpc.pb.h"
#include "grpc_security.hpp"

namespace pb = etherstack::callcontrol::v1;

class CallControlImpl final : public pb::CallControl::Service {
public:
    explicit CallControlImpl(ether::CallControlService& core) : core_(core) {}

    // Unary. Map domain errors to gRPC status codes - not to "accepted=false" for malformed input.
    grpc::Status StartCall(grpc::ServerContext*, const pb::StartCallRequest* req, pb::StartCallResponse* resp) override {
        ether::StartCallRequest r{req->call_id(), req->talkgroup(), req->caller()};
        auto res = core_.start_call(r);
        if (!res.accepted) {
            if (res.reason == "call already exists") return {grpc::StatusCode::ALREADY_EXISTS, res.reason};
            return {grpc::StatusCode::INVALID_ARGUMENT, res.reason};
        }
        resp->set_accepted(true);
        return grpc::Status::OK;
    }
    grpc::Status EndCall(grpc::ServerContext*, const pb::EndCallRequest* req, pb::EndCallResponse* resp) override {
        if (!core_.end_call(req->call_id())) return {grpc::StatusCode::NOT_FOUND, "no such call"};
        resp->set_ok(true);
        return grpc::Status::OK;
    }
    // Server streaming. The loop ends when: the client cancels or its DEADLINE expires (ctx->IsCancelled()),
    // the call ends (snapshot empty), or a Write fails.
    grpc::Status WatchStats(grpc::ServerContext* ctx, const pb::WatchStatsRequest* req, grpc::ServerWriter<pb::CallStats>* w) override {
        unsigned interval = req->interval_ms() ? req->interval_ms() : 1000;
        if (!core_.snapshot(req->call_id())) return {grpc::StatusCode::NOT_FOUND, "no such call"};
        while (!ctx->IsCancelled()) {
            auto s = core_.snapshot(req->call_id());
            if (!s) break;                                           // call ended: finish the stream with OK
            pb::CallStats out;
            out.set_call_id(s->call_id); out.set_packets(s->packets); out.set_lost(s->lost); out.set_jitter_ms(s->jitter_ms);
            if (!w->Write(out)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
        return grpc::Status::OK;
    }
private:
    ether::CallControlService& core_;
};

int main(int argc, char** argv) {
    std::string addr = argc > 1 ? argv[1] : "127.0.0.1:50051";
    ether::CallControlService core;
    CallControlImpl service(core);
    grpc::ServerBuilder b;
    b.AddListeningPort(addr, grpc::InsecureServerCredentials());     // DEV ONLY: use grpc::SslServerCredentials + mTLS in production
    b.RegisterService(&service);
    std::unique_ptr<grpc::Server> server = b.BuildAndStart();
    if (!server) { std::cerr << "failed to start on " << addr << "\n"; return 1; }
    std::cout << "CallControl listening on " << addr << std::endl;
    // Feed some synthetic packets so WatchStats has something to show (no live radio traffic).
    std::thread feeder([&] { for (int i = 0; i < 100000; ++i) { core.record_packet("demo", i % 50 == 0, 1.0 + (i % 7) * 0.1); std::this_thread::sleep_for(std::chrono::milliseconds(20)); } });
    feeder.detach();
    server->Wait();
}
