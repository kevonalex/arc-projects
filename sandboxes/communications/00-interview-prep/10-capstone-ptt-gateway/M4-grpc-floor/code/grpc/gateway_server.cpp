// Gateway gRPC server: a THIN adapter over gw::GatewayService (the logic you unit-tested).
// NOT compiled by default (needs gRPC + protobuf). UNTESTED in the authoring environment - VERIFY. See README.md.
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "gateway.grpc.pb.h"
#include "gateway_core.hpp"

namespace pb = etherstack::gateway::v1;

static grpc::Status to_grpc(const gw::Status& s) { return grpc::Status(static_cast<grpc::StatusCode>(s.code), s.message); }  // numbers match by design

class GatewayImpl final : public pb::Gateway::Service {
 public:
  grpc::Status StartCall(grpc::ServerContext*, const pb::StartCallRequest* req, pb::StartCallResponse* resp) override {
    std::lock_guard<std::mutex> lk(mu_);  // the core is NOT thread-safe: the adapter owns the lock
    gw::Status s = core_.start_call(req->call_id(), req->talkgroup(), req->caller());
    resp->set_message(s.message);
    return to_grpc(s);
  }
  grpc::Status EndCall(grpc::ServerContext*, const pb::EndCallRequest* req, pb::EndCallResponse*) override {
    std::lock_guard<std::mutex> lk(mu_);
    return to_grpc(core_.end_call(req->call_id()));
  }
  // Server streaming: ends when the client cancels / deadline expires, the call disappears, or a Write fails.
  grpc::Status WatchStats(grpc::ServerContext* ctx, const pb::WatchStatsRequest* req, grpc::ServerWriter<pb::CallStats>* w) override {
    const unsigned interval = req->interval_ms() ? req->interval_ms() : 1000;
    while (!ctx->IsCancelled()) {
      std::optional<gw::CallStats> s;
      {
        std::lock_guard<std::mutex> lk(mu_);
        s = core_.snapshot(req->call_id());
      }
      if (!s) return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such call");
      pb::CallStats out;
      out.set_call_id(s->call_id);
      out.set_packets(s->packets);
      out.set_lost(s->lost);
      out.set_loss_pct(s->loss_pct);
      out.set_ber(s->ber);
      out.set_jitter_ms(s->jitter_ms);
      if (!w->Write(out)) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
    return grpc::Status::OK;
  }
  // Bidirectional: read client messages in this thread, push floor events back after each one.
  // SKELETON: a real server also needs a timer thread that calls core_.tick() and pushes TimedOut/Granted events to every
  // subscriber of the call - that fan-out (one stream per user) is exercise W 5.x.
  grpc::Status FloorChannel(grpc::ServerContext*, grpc::ServerReaderWriter<pb::FloorMessage, pb::FloorMessage>* rw) override {
    pb::FloorMessage in;
    while (rw->Read(&in)) {
      std::vector<gw::FloorEvent> events;
      {
        std::lock_guard<std::mutex> lk(mu_);
        if (in.kind() == pb::FloorMessage::REQUEST) core_.floor_request(in.call_id(), in.user(), static_cast<int>(in.priority()));
        else if (in.kind() == pb::FloorMessage::RELEASE) core_.floor_release(in.call_id(), in.user());
        events = core_.floor_events(in.call_id());
      }
      for (const auto& e : events) {
        pb::FloorMessage out;
        out.set_call_id(in.call_id());
        out.set_user(e.user);
        switch (e.kind) {
          case gw::FloorEventKind::Granted: out.set_kind(pb::FloorMessage::GRANTED); break;
          case gw::FloorEventKind::Queued: out.set_kind(pb::FloorMessage::QUEUED); break;
          case gw::FloorEventKind::Revoked: out.set_kind(pb::FloorMessage::REVOKED); break;
          case gw::FloorEventKind::TimedOut: out.set_kind(pb::FloorMessage::TIMED_OUT); break;
          case gw::FloorEventKind::Idle: out.set_kind(pb::FloorMessage::IDLE); break;
          case gw::FloorEventKind::Released: continue;  // the releaser already knows
        }
        if (!rw->Write(out)) return grpc::Status::OK;
      }
    }
    return grpc::Status::OK;
  }

 private:
  std::mutex mu_;
  gw::GatewayService core_;
};

int main(int argc, char** argv) {
  const std::string addr = argc > 1 ? argv[1] : "127.0.0.1:50061";
  GatewayImpl service;
  grpc::ServerBuilder b;
  b.AddListeningPort(addr, grpc::InsecureServerCredentials());  // DEV ONLY: use SslServerCredentials + mTLS beyond localhost
  b.RegisterService(&service);
  std::unique_ptr<grpc::Server> server = b.BuildAndStart();
  if (!server) {
    std::cerr << "failed to start on " << addr << "\n";
    return 1;
  }
  std::cout << "Gateway listening on " << addr << std::endl;
  server->Wait();
}
