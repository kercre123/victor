/*
 * File:          cozmoEngine.cpp
 * Date:          12/23/2014
 *
 * Description:   (See header file.)
 *
 * Author: Andrew Stein / Kevin Yoon
 *
 * Modifications:
 */

#include "anki/cozmo/basestation/cozmoEngineClient.h"
#include "anki/cozmo/basestation/engineImpl/cozmoEngineClientImpl.h"

namespace Anki {
namespace Cozmo {
  

CozmoEngineClient::CozmoEngineClient(IExternalInterface* externalInterface, Data::DataPlatform* dataPlatform)
: CozmoEngine(externalInterface, dataPlatform)
{
  _clientImpl = new CozmoEngineClientImpl(_externalInterface, dataPlatform);
  assert(_clientImpl != nullptr);
  _impl = _clientImpl;
}

CozmoEngineClient::~CozmoEngineClient()
{

}

bool CozmoEngineClient::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
{
  PRINT_NAMED_WARNING("CozmoEngineClient.GetCurrentRobotImage", "Cannot yet request an image from robot on client.\n");
  return false;
}
  
  
} // namespace Cozmo
} // namespace Anki