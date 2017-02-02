/**
 * File: AnimationTriggerResponseContainer.h
 *
 * Authors: Molly Jameson
 * Created: 2016-4-4
 *
 * Description: Stores Responses to generic AnimationTriggers
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_Events_AnimationTriggerResponsesContainer_H__
#define __Cozmo_Basestation_Events_AnimationTriggerResponsesContainer_H__

#include "clad/types/animationTrigger.h"
#include <unordered_map>

namespace Anki {
  namespace Util
  {
    namespace Data
    {
      class DataPlatform;
    }
  }
  
namespace Cozmo {
  
  class AnimationTriggerResponsesContainer
  {
  public:
    bool        Load(const Anki::Util::Data::DataPlatform* data, std::string path);
    
    template<class AnimTrigger>
    std::string GetResponse(AnimTrigger ev);
    
    template<class AnimTrigger>
    bool HasResponse(AnimTrigger ev);
    
  private:
    
    std::unordered_map<std::string, std::string> _eventMap;
    
  }; // class AnimationTriggerResponsesContainer
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Events_AnimationTriggerResponsesContainer_H__
