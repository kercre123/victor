/**
 * File: animationGroupEntry.cpp
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Class for storing an animation selection
 *    Which defines a set of mood score graphs
 *    by which to evaluate the suitability of this animation
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/animationGroup/animationGroupEntry.h"
#include "util/logging/logging.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/moodSystem/simpleMoodTypesHelpers.h"

namespace Anki {
  namespace Cozmo {
    
    static const char* kNameKey = "Name";
    static const char* kWeightKey = "Weight";
    static const char* kMoodKey = "Mood";
    
    AnimationGroupEntry::AnimationGroupEntry()
    {
    }
    
    Result AnimationGroupEntry::DefineFromJson(const Json::Value &jsonRoot)
    {
      const Json::Value& jsonName = jsonRoot[kNameKey];
      
      if(!jsonName.isString()) {
        PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.NoName",
                          "Missing '%s' field for animation.", kNameKey);
        return RESULT_FAIL;
      }
      
      _name = jsonName.asString();
      
      const Json::Value& jsonWeight = jsonRoot[kWeightKey];
      
      if(!jsonWeight.isDouble()) {
        PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.NoWeight",
                          "Missing '%s' field for animation.", kWeightKey);
        
        return RESULT_FAIL;
      }
      
      _weight = jsonWeight.asFloat();
      
      const Json::Value& jsonMood = jsonRoot[kMoodKey];
      
      if(!jsonMood.isString()) {
        PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.NoMood",
                          "Missing '%s' field for animation.", kMoodKey);
        
        return RESULT_FAIL;
      }
      
      const char* moodTypeString = jsonMood.asCString();
      _mood  = SimpleMoodTypeFromString( moodTypeString );
      
      if (_mood == SimpleMoodType::Count)
      {
        PRINT_NAMED_WARNING("SimpleMoodScorer.ReadFromJson.BadType", "Bad '%s' = '%s'", kMoodKey, moodTypeString);
        return RESULT_FAIL;
      }
      
      
      return RESULT_OK;
    }
    
  } // namespace Cozmo
} // namespace Anki
