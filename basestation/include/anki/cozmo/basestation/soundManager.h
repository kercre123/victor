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
#include <unordered_map>

namespace Anki {
  namespace Cozmo {
    
    // NOTE: this is a singleton class
    class SoundManager
    {
    public:
      
      // Get a pointer to the singleton instance
      inline static SoundManager* getInstance();
      static void removeInstance();
      
      // Clears all available sounds
      void Clear();
      
      // Get the total number of sounds currently available in SoundManager
      size_t GetNumAvailableSounds() const;
      
      // Set the root directory of sound files and causes a (re-)read of available
      // sound files there.
      bool SetRootDir(const char* dir);
      
      // Volume for Play() methods is expressed a percentage of saved volume
      bool Play(const std::string& name, const u8 numLoops=1, const u8 volume = 100);
      
	    void Stop();
      
      // Return true if the given name is an available sound
      bool IsValidSound(const std::string& name) const;

      // Get the duration of the sound in milliseconds. Returns 0 for invalid names.
      const u32 GetSoundDurationInMilliseconds(const std::string& name) const;
      
    protected:
      
      // Protected default constructor for singleton.
      SoundManager();
      ~SoundManager();
      
      static SoundManager* singletonInstance_;
      
      bool _hasCmdProcessor;
      bool _hasRootDir;
      
      std::string _rootDir;
      
      Result ReadSoundDir(std::string subDir = "");
      
      struct AvailableSound {
        time_t lastLoadedTime;
        u32    duration_ms;
        std::string fullFilename;
      };
      
      std::unordered_map<std::string, AvailableSound> _availableSounds;
      
    }; // class SoundManager
    
    
    inline SoundManager* SoundManager::getInstance()
    {
      // If we haven't already instantiated the singleton, do so now.
      if(0 == singletonInstance_) {
        singletonInstance_ = new SoundManager();
      }
      
      return singletonInstance_;
    }

    inline void SoundManager::Clear() {
      _availableSounds.clear();
    }
    
    inline size_t SoundManager::GetNumAvailableSounds() const {
      return _availableSounds.size();
    }
    
  } // namespace Cozmo
} // namespace Anki


#endif // SOUND_MANAGER_H
