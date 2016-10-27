/**
 * File: AnimationTriggerResponseContainer.cpp
 *
 * Authors: Molly Jameson
 * Created: 2016-4-4
 *
 * Description: Stores Responses to generic AnimationTriggers
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/events/animationTriggerResponsesContainer.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/fileUtils/fileUtils.h"
#include "json/json.h"
#include "util/logging/logging.h"

namespace Anki
{
namespace Cozmo
{
  
  bool AnimationTriggerResponsesContainer::Load(const Anki::Util::Data::DataPlatform* data, std::string path)
  {
    if (nullptr == data )
    {
      return false;
    }
    const std::string map_dir = data->pathToResource(Util::Data::Scope::Resources, path);
    
    auto filePaths = Anki::Util::FileUtils::FilesInDirectory(map_dir, true, "json", true);
    // Read all json files in that dir..
    for (const auto& filename : filePaths)
    {
      Json::Value jsonRoot;
      const bool success = data->readAsJson(filename, jsonRoot);
      if( success && jsonRoot.isMember("Pairs"))
      {
        Json::Value allPairsArray = jsonRoot["Pairs"];
        const int numPairs = allPairsArray.size();
        for(int i = 0; i < numPairs; ++i)
        {
          const Json::Value& singleEvent = allPairsArray[i];
          // store the string since we don't have a string -> Enum function and will end up with a lot of these
          _eventMap.emplace(singleEvent["CladEvent"].asString(), singleEvent["AnimName"].asString());
        }
      }
    }
    
    return true;
  }

  std::string AnimationTriggerResponsesContainer::GetResponse(Anki::Cozmo::AnimationTrigger ev)
  {
    auto retVal = _eventMap.find(EnumToString(ev));
    if(retVal == _eventMap.end())
    {
      PRINT_NAMED_ERROR("AnimationTriggerResponsesContainer::GetResponse",
                        "Animation requested for unknown response '%s'",
                        AnimationTriggerToString(ev));
      return "";
    }

    PRINT_CH_INFO("Animations", "GetResponseForAnimationTrigger.Found",
                  "%s -> %s",
                  AnimationTriggerToString(ev),
                  retVal->second.c_str());
    
    return retVal->second;
  }
  
  bool AnimationTriggerResponsesContainer::HasResponse(Anki::Cozmo::AnimationTrigger ev)
  {
    auto retVal = _eventMap.find(EnumToString(ev));
    return retVal != _eventMap.end();
  }


}
}
