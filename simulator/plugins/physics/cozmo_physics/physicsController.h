#ifndef __CozmoPhysics_PhysicsControllerImpl_H__
#define __CozmoPhysics_PhysicsControllerImpl_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include <plugins/physics.h>
#pragma GCC diagnostic pop
#include <tuple>
#include <vector>
#include <unordered_map>
#include <string>
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/messaging/shared/UdpServer.h"
#include "clad/physicsInterface/messageSimPhysics.h"



namespace Anki {
namespace Cozmo {

class PhysicsController
{
public:
  PhysicsController() {}

  ///
  // @brief      Checks the server for any recieved packets and pass it to ProcessMessage after
  //             casting it into a MessageSimPhysics message.
  //
  void Init();
  void Update();
  void Cleanup();

private:
  ///
  // @brief      Broadcasts recieved message to _eventManager. There's nothing special about the
  //             _eventManager and this is essentially the same as a switch case with the message
  //             and call the appropriate functions the message is bound to in the event manager. It
  //             is done this way to maintain consistency with physVizController.cpp
  // @param[in]  message  message to broadcast.
  //
  void ProcessMessage(PhysicsInterface::MessageSimPhysics&& message);

  void Subscribe(
    const PhysicsInterface::MessageSimPhysicsTag& tagType,
    std::function<void(const AnkiEvent<PhysicsInterface::MessageSimPhysics>&)> messageHandler)
  {
    _eventManager.SubscribeForever(static_cast<uint32_t>(tagType), messageHandler);
  }

  void ProcessApplyForceMessage(const AnkiEvent<PhysicsInterface::MessageSimPhysics>& msg);

  void SetLinearVelocity(const std::string objectName, const Point<3, float> velVector);

  // ODE functions reference solid objects in the world by a ID called dBodyID. This function
  // fetches the ID, given the DEF name in the .wbt file, and stores it in a map the first time the
  // body ID is requested. The stored value is returned on subsequent calls for the same object.
  const dBodyID GetdBodyID(const std::string& objectName);

  std::unordered_map<std::string, dBodyID> _dBodyIDMap;

  AnkiEventMgr<PhysicsInterface::MessageSimPhysics> _eventManager;
  UdpServer _server;
};

}  // namespace Cozmo
}  // namespace Anki

#endif  //__CozmoPhysics_PhysicsControllerImpl_H__
