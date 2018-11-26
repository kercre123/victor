/**
 * File: protoCladInterpreter.h
 *
 * Author: Ron Barry
 * Date:   11/16/2018
 *
 * Description: Determine which proto messages need to be converted to
 *              Clad before being dispatched to their final destinations.
 *              (The gateway no longer does this work.)
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"

#include "clad/externalInterface/messageGameToEngine.h"
#include "proto/external_interface/shared.pb.h"

namespace Anki {
namespace Vector {


class ProtoCladInterpreter {
public:
    static bool Redirect(const external_interface::GatewayWrapper & message, CozmoContext * cozmo_context);
    static bool Redirect(const ExternalInterface::MessageGameToEngine & message, CozmoContext * cozmo_context);

private:
  ProtoCladInterpreter() {}

  //
  // Proto-to-Clad interpreters
  //
  static void ProtoDriveWheelsRequestToClad(
      const external_interface::GatewayWrapper & proto_message,
      ExternalInterface::MessageGameToEngine & clad_message);

  static void ProtoPlayAnimationRequestToClad(
      const external_interface::GatewayWrapper & proto_message,
      ExternalInterface::MessageGameToEngine & clad_message);

  static void ProtoListAnimationsRequestToClad(
      const external_interface::GatewayWrapper & proto_message,
      ExternalInterface::MessageGameToEngine & clad_message);

  //
  // Clad-to-Proto interpreters
  //
  static void CladDriveWheelsToProto(
      const ExternalInterface::MessageGameToEngine & clad_message,
      external_interface::GatewayWrapper & proto_message);

  static void CladPlayAnimationToProto(
      const ExternalInterface::MessageGameToEngine & clad_message,
      external_interface::GatewayWrapper & proto_message);

  static void CladListAnimationsToProto(
      const ExternalInterface::MessageGameToEngine & clad_message,
      external_interface::GatewayWrapper & proto_message);

};

} // namespace Vector
} // namespace Anki
