/**
 * File: cubeBleClient_android.cpp
 *
 * Author: Matt Michini
 * Created: 12/1/2017
 *
 * Description:
 *               Defines interface to BLE central process which communicates with cubes
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cubeBleClient.h"

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using cubeBleClient_android.cpp
#endif

namespace Anki {
namespace Cozmo {


CubeBleClient::CubeBleClient()
{

}

// Returns the single instance of the object.
CubeBleClient* CubeBleClient::GetInstance()
{
  return getInstance();
}


Result CubeBleClient::Update()
{
  return RESULT_OK;
}


bool CubeBleClient::SendMessageToLightCube(const BleFactoryId&, const MessageEngineToCube&)
{
  return true;
}


bool CubeBleClient::ConnectToCube(const BleFactoryId&)
{
  return true;
}


bool CubeBleClient::DisconnectFromCube(const BleFactoryId&)
{
  return true;
}

} // namespace Cozmo
} // namespace Anki
