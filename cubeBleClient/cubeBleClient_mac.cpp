/**
 * File: cubeBleClient_mac.cpp
 *
 * Author: Matt Michini
 * Created: 12/1/2017
 *
 * Description:
 *               Defines interface to simulated cubes (mac-specific implementations)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef WEBOTS

#include "cubeBleClient.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/externalInterface/messageCubeToEngine.h"
#include "clad/externalInterface/messageEngineToCube.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Vector {

int GetReceiverChannel(const BleFactoryId& factoryId)
{
  const int channel = (int) (std::hash<std::string>{}(factoryId) & 0x3FFFFFFF);
  return channel;
}
  
int GetEmitterChannel(const BleFactoryId& factoryId)
{
  return 1 + GetReceiverChannel(factoryId);
}


CubeBleClient::CubeBleClient()
{
}


CubeBleClient::~CubeBleClient()
{

}


void CubeBleClient::SetScanDuration(const float duration_sec)
{
}


void CubeBleClient::SetCubeFirmwareFilepath(const std::string& path)
{
  // not implemented for mac
}


void CubeBleClient::StartScanInternal()
{
}


void CubeBleClient::StopScanInternal()
{
}


bool CubeBleClient::SendMessageInternal(const MessageEngineToCube& msg)
{
  return false;
}
  
  
bool CubeBleClient::RequestConnectInternal(const BleFactoryId& factoryId)
{
  return true;
}


bool CubeBleClient::RequestDisconnectInternal()
{
  return true;
}

  
bool CubeBleClient::InitInternal()
{
  return true;
}

  
bool CubeBleClient::UpdateInternal()
{
  return true;
}

  
} // namespace Vector
} // namespace Anki

#endif // WEBOTS

