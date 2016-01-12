/**
 * File: animationGroupEntry.h
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Class for storing an animation select
 *    Which defines a set of mood animation curves
 *    by which to evaluate the suitability of this animation
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/animationGroup/animationGroupEntry.h"
#include "util/logging/logging.h"
//#include <cassert>

#define DEBUG_ANIMATIONS 0

namespace Anki {
  namespace Cozmo {
    
    AnimationGroupEntry::AnimationGroupEntry()
    {
    }
    
    Result AnimationGroupEntry::DefineFromJson(Json::Value &jsonRoot)
    {
      if(!jsonRoot.isMember("Name")) {
        PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.NoName",
                          "Missing 'Name' field for animation.");
        return RESULT_FAIL;
      }
      
      _name = jsonRoot["Name"].asString();
      
      if(jsonRoot.isMember("MoodCurves")) {
        
        Json::Value& jsonMoodCurves = jsonRoot["MoodCurves"];
        
        
        const s32 numEntries = jsonMoodCurves.size();
        for(s32 i = 0; i < numEntries; ++i)
        {
          Json::Value& jsonMoodCurve = jsonMoodCurves[i];
          
          _moodCurves.push_back(AnimationGroupMoodCurve());
          
          auto addResult = _moodCurves.back().DefineFromJson(jsonMoodCurve);
          
          if(addResult != RESULT_OK) {
            PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.AddMoodCurveFailure",
                              "Adding mood curve %d for %s failed.",
                              i,
                              _name.c_str());
            return addResult;
          }
          
        } // for each Mood Curve
      }
      
      
      return RESULT_OK;
    }
    
    
    float AnimationGroupEntry::Evaluate(const MoodManager& moodManager) const {
      if(_moodCurves.size() == 0)
        return 0;
      
      float value = 0;
      for(auto it = _moodCurves.begin(); it != _moodCurves.end(); it++) {
        value += it->Evaluate(moodManager.GetEmotionValue((EmotionType)it->GetEmotion()));
      }
      
      return value / _moodCurves.size();
    }
    
  } // namespace Cozmo
} // namespace Anki
