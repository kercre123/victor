/**
 * File: gameEventResponseContainer.h
 *
 * Authors: Molly Jameson
 * Created: 2016-4-4
 *
 * Description: Stores Responses to generic gameevents
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_Events_GameEventResponsesContainer_H__
#define __Cozmo_Basestation_Events_GameEventResponsesContainer_H__

#include "clad/types/gameEvent.h"
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
  
  class GameEventResponsesContainer
  {
  public:
    bool        Load(Anki::Util::Data::DataPlatform* data, std::string path);
    
    std::string GetResponse(Anki::Cozmo::GameEvent ev);
    bool        HasResponse(Anki::Cozmo::GameEvent ev);
    
  private:
    std::unordered_map<std::string, std::string> _eventMap;
    
  }; // class GameEventResponsesContainer
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Events_GameEventResponsesContainer_H__
