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

namespace Anki {
  namespace Cozmo {
    
    // List of sound schemes
    typedef enum {
      SOUND_SCHEME_COZMO
      ,SOUND_SCHEME_MOVIE
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
      ,NUM_SOUNDS
    } SoundID_t;
    
    
    // NOTE: this is a singleton class
    class SoundManager
    {
    public:
      
      // Get a pointer to the singleton instance
      inline static SoundManager* getInstance();
      
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
