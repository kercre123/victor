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

#ifndef __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineClientImpl_H__
#define __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineClientImpl_H__

#include "anki/cozmo/basestation/engineImpl/cozmoEngineImpl.h"

namespace Anki {
namespace Cozmo {
  
class CozmoEngineClientImpl : public CozmoEngineImpl
{
public:
  CozmoEngineClientImpl(IExternalInterface* externalInterface, Util::Data::DataPlatform* dataPlatform);

protected:

  virtual Result InitInternal() override;
  virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) override;

}; // class CozmoEngineClientImpl


} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineClientImpl_H__
