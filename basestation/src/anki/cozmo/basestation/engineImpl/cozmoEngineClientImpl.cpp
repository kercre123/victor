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

#include "anki/cozmo/basestation/engineImpl/cozmoEngineClientImpl.h"

namespace Anki {
namespace Cozmo {

CozmoEngineClientImpl::CozmoEngineClientImpl(IExternalInterface* externalInterface,
                                             Util::Data::DataPlatform* dataPlatform)
: CozmoEngineImpl(externalInterface, dataPlatform)
{

}

Result CozmoEngineClientImpl::InitInternal()
{
  // TODO: Do client-specific init here

  return RESULT_OK;
}

Result CozmoEngineClientImpl::UpdateInternal(const BaseStationTime_t currTime_ns)
{
  // TODO: Do client-specific update stuff here

  return RESULT_OK;
} // UpdateInternal()
  

} // namespace Cozmo
} // namespace Anki