// TODO: Add header

#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "sdk_interface.grpc.pb.h"
#include "switchboardd/engineMessagingClient.h"
#include "sdkHandler.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using std::chrono::system_clock;

namespace Anki {
namespace Switchboard {

class SdkServer final {
public:
  explicit SdkServer(std::shared_ptr<EngineMessagingClient> emc);
  ~SdkServer();

  void Run();
private:
  SdkHandler service;
  std::unique_ptr<Server> _server;
  std::shared_ptr<EngineMessagingClient> _engineMessagingClient;
};

} // Switchboard
} // Anki






