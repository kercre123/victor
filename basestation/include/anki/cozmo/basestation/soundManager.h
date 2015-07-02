/**
 * File: soundManager.h
 *
 * Author: Kevin Yoon
 * Date:   6/18/2014
 *
 * Description: Simple sound player, that only plays one sound at a time and only works on mac.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "anki/cozmo/shared/cozmoTypes.h"

#include <string>

namespace Anki {
  namespace Cozmo {
    
    
    // List of sound schemes
    typedef enum {
      SOUND_SCHEME_COZMO
      ,SOUND_SCHEME_MOVIE
      ,SOUND_SCHEME_CREEP // Cozmo Robotic Emotional Engagement Playtest
      ,NUM_SOUND_SCHEMES
    } SoundSchemeID_t;
    
    // List of sound IDs
    typedef enum {
      SOUND_TADA
      ,SOUND_NOPROBLEMO
      ,SOUND_INPUT
      ,SOUND_SWEAR
      ,SOUND_STARTOVER
      ,SOUND_NOTIMPRESSED
      ,SOUND_60PERCENT
      ,SOUND_DROID
      ,SOUND_DEMO_START
      ,SOUND_WAITING4DICE
      ,SOUND_WAITING4DICE2DISAPPEAR
      ,SOUND_OK_GOT_IT
      ,SOUND_OK_DONE
      ,SOUND_POWER_ON
      ,SOUND_POWER_DOWN
      ,SOUND_PHEW
      ,SOUND_OOH
      ,SOUND_SCREAM
      ,SOUND_WHATS_NEXT
      ,SOUND_HELP_ME
      ,SOUND_SCAN
      ,SOUND_HAPPY_CHASE
      ,SOUND_FLEES
      ,SOUND_SINGING
      ,SOUND_SHOOT
      ,SOUND_SHOT_HIT
      ,SOUND_SHOT_MISSED
      ,NUM_SOUNDS
    } SoundID_t;
    
    // NOTE: this is a singleton class
    class SoundManager
    {
    public:
      
      // Get a pointer to the singleton instance
      inline static SoundManager* getInstance();
      static void removeInstance();
      
      // Set the root directory of sound files
      bool SetRootDir(const char* dir);
      
      // Volume for Play() methods is expressed a percentage of saved volume
      bool Play(const SoundID_t    id,   const u8 numLoops=1, const u8 volume = 100);
      bool Play(const std::string& name, const u8 numLoops=1, const u8 volume = 100);
      
	    void Stop();

      void SetScheme(const SoundSchemeID_t scheme);
      SoundSchemeID_t GetScheme() const;
      
      // Lookup a sound by name
      static SoundID_t GetID(const std::string& name);
      
      // Lookup a relative filename by ID
      const std::string& GetSoundFile(SoundID_t soundID) const;
      
      // Get the duration of the sound in milliseconds
      const u32 GetSoundDurationInMilliseconds(SoundID_t soundID) const;
      const u32 GetSoundDurationInMilliseconds(const std::string& name) const;
      
    protected:
      
      // Protected default constructor for singleton.
      SoundManager();
      ~SoundManager();
      
      static SoundManager* singletonInstance_;
      
      bool _hasCmdProcessor;
      bool _hasRootDir;
      
      std::string _rootDir;
      
      SoundSchemeID_t _currScheme = SOUND_SCHEME_CREEP;
      
    }; // class SoundManager
    
    
    inline SoundManager* SoundManager::getInstance()
    {
      // If we haven't already instantiated the singleton, do so now.
      if(0 == singletonInstance_) {
        singletonInstance_ = new SoundManager();
      }
      
      return singletonInstance_;
    }
    
    inline const u32 SoundManager::GetSoundDurationInMilliseconds(const std::string& name) const {
      return GetSoundDurationInMilliseconds(GetID(name));
    }
    
  } // namespace Cozmo
} // namespace Anki


#endif // SOUND_MANAGER_H
