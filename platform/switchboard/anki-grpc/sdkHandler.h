// TODO: Header

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
#include "switchboardd/log.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
// #include "clad/externalInterface/messageExternalComms.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using sdkinterface::WheelMotors;
using sdkinterface::Event;
using sdkinterface::GenericResult;
using sdkinterface::Animation;
using sdkinterface::AnimationList;
using sdkinterface::ListOptions;
using sdkinterface::SDKInterface;
using std::chrono::system_clock;

using G2EMessageTag = Anki::Cozmo::ExternalInterface::MessageGameToEngineTag;
using G2EMessage = Anki::Cozmo::ExternalInterface::MessageGameToEngine;
using E2GMessageTag = Anki::Cozmo::ExternalInterface::MessageEngineToGameTag;
using E2GMessage = Anki::Cozmo::ExternalInterface::MessageEngineToGame;
// using ExtCommsMessageTag = Anki::Cozmo::ExternalComms::ExternalCommsTag;
// using ExtCommsMessage = Anki::Cozmo::ExternalComms::ExternalComms;

namespace Anki {
namespace Switchboard {

/*
 *    TODO: NOT THIS vvvvvv
 */

// TODO: remove this
// std::string GetFeatureName(const Point& point,
//                            const std::vector<Feature>& feature_list) {
//   for (const Feature& f : feature_list) {
//     if (f.location().latitude() == point.latitude() &&
//         f.location().longitude() == point.longitude()) {
//       return f.name();
//     }
//   }
//   return "";
// }

class SdkHandler final : public SDKInterface::Service {
private:
  std::shared_ptr<EngineMessagingClient> _engineMessagingClient;
public:
  explicit SdkHandler(std::shared_ptr<EngineMessagingClient> emc)
  : _engineMessagingClient(emc)
  {
    
  }

  Status PlayAnimation(ServerContext* context, const Animation* animation,
                    GenericResult* result) override {
    logi("Shawn Test: PlayAnimation Start");
    result->set_description("this is a description");
    // feature->set_name(GetFeatureName(*point, feature_list_));
    // feature->mutable_location()->CopyFrom(*point);
    
    // Anki::Cozmo::ExternalInterface::DriveWheels engineMessage;
    // engineMessage.lwheel_speed_mmps = motors->left_wheel_mmps();
    // engineMessage.rwheel_speed_mmps = motors->right_wheel_mmps();
    // engineMessage.lwheel_accel_mmps2 = motors->left_wheel_mmps2();
    // engineMessage.rwheel_accel_mmps2 = motors->right_wheel_mmps2();
    // auto msg = G2EMessage::CreateDriveWheels(std::move(engineMessage));
    // _engineMessagingClient->SendMessage(msg);

    logi("Shawn Test: PlayAnimation End");
    return Status::OK;
  }

  Status DriveWheels(ServerContext* context, const WheelMotors* motors,
                    GenericResult* result) override {
    logi("Shawn Test: DriveWheels Start");
    result->set_description("this is a description");
    Anki::Cozmo::ExternalInterface::DriveWheels engineMessage;
    engineMessage.lwheel_speed_mmps =  /** /200.0;//*/motors->left_wheel_mmps();
    engineMessage.rwheel_speed_mmps =  /** /200.0;//*/motors->right_wheel_mmps();
    engineMessage.lwheel_accel_mmps2 = /** /200.0;//*/motors->left_wheel_mmps2();
    engineMessage.rwheel_accel_mmps2 = /** /200.0;//*/motors->right_wheel_mmps2();
    auto msg = G2EMessage::CreateDriveWheels(std::move(engineMessage));
    _engineMessagingClient->SendMessage(msg);
    // _engineMessagingClient->Test();

    // feature->set_name(GetFeatureName(*point, feature_list_));
    // feature->mutable_location()->CopyFrom(*point);

    logi("Shawn Test: DriveWheels End");
    return Status::OK;
  }

  // TODO: this and GetFeature if I want to do a one event stream and
  //       a separate direct response rpc
  Status ListAnimations(ServerContext* context,
                      const ListOptions* options,
                      AnimationList* animList) override {
    logi("Shawn Test: ListAnimations");
    // auto lo = rectangle->lo();
    // auto hi = rectangle->hi();
    // long left = (std::min)(lo.longitude(), hi.longitude());
    // long right = (std::max)(lo.longitude(), hi.longitude());
    // long top = (std::max)(lo.latitude(), hi.latitude());
    // long bottom = (std::min)(lo.latitude(), hi.latitude());
    // for (const Feature& f : feature_list_) {
    //   if (f.location().longitude() >= left &&
    //       f.location().longitude() <= right &&
    //       f.location().latitude() >= bottom &&
    //       f.location().latitude() <= top) {
    //     writer->Write(f);
    //   }
    // }
    return Status::OK;
  }

  // TODO: this and GetFeature if I want to do a one event stream and
  //       a separate direct response rpc
  Status EventStream(ServerContext* context,
                      const ListOptions* options,
                      ServerWriter<Event>* writer) override {
    logi("Shawn Test: EventStream");
    // auto lo = rectangle->lo();
    // auto hi = rectangle->hi();
    // long left = (std::min)(lo.longitude(), hi.longitude());
    // long right = (std::max)(lo.longitude(), hi.longitude());
    // long top = (std::max)(lo.latitude(), hi.latitude());
    // long bottom = (std::min)(lo.latitude(), hi.latitude());
    // for (const Feature& f : feature_list_) {
    //   if (f.location().longitude() >= left &&
    //       f.location().longitude() <= right &&
    //       f.location().latitude() >= bottom &&
    //       f.location().latitude() <= top) {
    //     writer->Write(f);
    //   }
    // }
    return Status::OK;
  }

//   Status RecordRoute(ServerContext* context, ServerReader<Point>* reader,
//                      RouteSummary* summary) override {
//     Point point;
//     int point_count = 0;
//     int feature_count = 0;
//     float distance = 0.0;
//     Point previous;

//     system_clock::time_point start_time = system_clock::now();
//     while (reader->Read(&point)) {
//       point_count++;
//       if (!GetFeatureName(point, feature_list_).empty()) {
//         feature_count++;
//       }
//       if (point_count != 1) {
//         distance += GetDistance(previous, point);
//       }
//       previous = point;
//     }
//     system_clock::time_point end_time = system_clock::now();
//     summary->set_point_count(point_count);
//     summary->set_feature_count(feature_count);
//     summary->set_distance(static_cast<long>(distance));
//     auto secs = std::chrono::duration_cast<std::chrono::seconds>(
//         end_time - start_time);
//     summary->set_elapsed_time(secs.count());

//     return Status::OK;
//   }

// // TODO: This if I'm doing a stream to stream
//   Status StreamMessages(ServerContext* context,
//                    ServerReaderWriter<RobotToEngine, EngineToRobot>* stream) override {
//     std::vector<RobotToEngine> received_msgs;
//     RobotToEngine msg;
//     while (stream->Read(&msg)) {
//       for (const RobotToEngine& m : received_msgs) {
//         if (m.location().latitude() == msg.location().latitude() &&
//             m.location().longitude() == msg.location().longitude()) {
//           stream->Write(n);
//         }
//       }
//       received_msgs.push_back(msg);
//     }

//     return Status::OK;
//   }

 private:

//   std::vector<Feature> feature_list_;
};
/*
 *    TODO: NOT THIS ^^^^^^
 */


} // Switchboard
} // Anki