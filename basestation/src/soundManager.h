/**
 * File: soundManager.h
 *
 * Author: Kevin Yoon
 * Date:   6/18/2014
 *
 * Description: Simple sound player, that only works on mac.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "anki/cozmo/shared/cozmoTypes.h"

namespace Anki {
  namespace Cozmo {
    
    // NOTE: this is a singleton class
    class SoundManager
    {
    public:
      
      // Get a pointer to the singleton instance
      inline static SoundManager* getInstance();
      static void removeInstance();
      
      // Set the root directory of sound files
      bool SetRootDir(const char* dir);
      
      bool Play(const SoundID_t id);
      
      void SetScheme(const SoundSchemeID_t scheme);
      SoundSchemeID_t GetScheme() const;
      
    protected:
      
      // Protected default constructor for singleton.
      SoundManager();
      
      static SoundManager* singletonInstance_;

    }; // class SoundManager
    
    
    inline SoundManager* SoundManager::getInstance()
    {
      // If we haven't already instantiated the singleton, do so now.
      if(0 == singletonInstance_) {
        singletonInstance_ = new SoundManager();
      }
      
      return singletonInstance_;
    }
    
  } // namespace Cozmo
} // namespace Anki


#endif // SOUND_MANAGER_H
