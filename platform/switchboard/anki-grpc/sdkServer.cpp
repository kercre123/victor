// TODO: fill out file header

#include "sdkServer.h"
#include "switchboardd/log.h"

namespace Anki {
namespace Switchboard {

SdkServer::SdkServer(std::shared_ptr<EngineMessagingClient> emc)
: service(emc),
  _engineMessagingClient(emc)
{}

SdkServer::~SdkServer() {
  _server->Shutdown();
}

void SdkServer::Run() {
  std::string server_address("0.0.0.0:80"); // TODO: kServerAddress

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials()); // TODO: use TLS
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  _server = std::move(server);
  Log::Write("Switchboard connection listening on %s", server_address.c_str());
//   server->Wait(); // TODO: do we not need to "Wait"? I believe not
}


} // Switchboard
} // Anki