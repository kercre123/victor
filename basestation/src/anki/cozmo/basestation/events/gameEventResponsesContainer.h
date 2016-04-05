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

#include "clad/types/gameEvents.h"
#include <unordered_map>

 namespace std {
  
  template <>
  struct hash<Anki::Cozmo::GameEvent>
  {
    std::size_t operator()(const Anki::Cozmo::GameEvent& k) const
    {
      using std::hash;
      return std::hash<uint8_t>()((uint8_t)k);
    }
  };
  
}

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
    GameEventResponsesContainer();
    bool Load(Anki::Util::Data::DataPlatform* data, std::string path);
    
    std::string GetResponse(Anki::Cozmo::GameEvent ev);
    bool        HasResponse(Anki::Cozmo::GameEvent ev);
    
  private:
    GameEvent   StringToEnum(std::string str);
    std::unordered_map<Anki::Cozmo::GameEvent, std::string> _eventMap;
    
  }; // class GameEventResponsesContainer
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
