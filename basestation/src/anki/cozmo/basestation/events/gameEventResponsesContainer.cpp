/**
 * File: gameEventResponseContainer.cpp
 *
 * Authors: Molly Jameson
 * Created: 2016-4-4
 *
 * Description: Stores Responses to generic gameevents
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/events/gameEventResponsesContainer.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/fileUtils/fileUtils.h"
#include "json/json.h"
#include "util/logging/logging.h"

namespace Anki
{
namespace Cozmo
{
  GameEventResponsesContainer::GameEventResponsesContainer()
  {
    // Because we don't have a fast "string to enum" function like loading would need
    // Just populate everything
  }
  
  bool GameEventResponsesContainer::Load(Anki::Util::Data::DataPlatform* data, std::string path)
  {
    if (nullptr == data )
    {
      return false;
    }
    const std::string map_dir = data->pathToResource(Util::Data::Scope::Resources, path);
    
    auto filePaths = Anki::Util::FileUtils::FilesInDirectory(map_dir, true, "json", true);
    // Read all json files in that dir..
    for (auto filename : filePaths)
    {
      Json::Value jsonRoot;
      const bool success = data->readAsJson(filename, jsonRoot);
      if( success )
      {
        // The object "pairs"
        if(jsonRoot.isMember("Pairs"))
        {
          Json::Value allPairsArray = jsonRoot["Pairs"];
          const int numFrames = allPairsArray.size();
          for(int iFrame = 0; iFrame < numFrames; ++iFrame)
          {
            const Json::Value& singleEvent = allPairsArray[iFrame];
            _eventMap.insert(singleEvent["CladEvent"], singleEvent["AnimName"]);
          }
        }
      }
    }
    
    return true;
  }

  std::string GameEventResponsesContainer::GetResponse(Anki::Cozmo::GameEvent ev)
  {
    auto retVal = _eventMap.find(ev);
    if(retVal == _eventMap.end())
    {
      PRINT_NAMED_ERROR("GameEventResponsesContainer::GetResponse",
                        "Animation requested for unknown animation '%s'.\n",
                        GameEventToString(ev));
      return "ANIM_TEST";
    }
    return retVal->second;
  }
  bool        GameEventResponsesContainer::HasResponse(Anki::Cozmo::GameEvent ev)
  {
    auto retVal = _eventMap.find(ev);
    return retVal != _eventMap.end();
  }
  
  // TODO: Build this map at the start.
  /*GameEvent   GameEventResponsesContainer::StringToEnum(std::string str)
  {
    int num_enums = (int)GameEvent::Count;
    for(int i = 0; i < num_enums; ++i)
    {
      if( EnumToString((GameEvent)i) == str)
      {
      }
    }
  }*/

}
}