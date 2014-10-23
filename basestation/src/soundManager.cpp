/**
 * File: soundManager.cpp
 *
 * Author: Kevin Yoon
 * Date:   6/18/2014
 *
 * Description: Simple sound player, that only works on mac.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "soundManager.h"
#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/platformPathManager.h"

#include <thread>
#include <map>

#include <sys/types.h>
#include <sys/stat.h>

#define MAX_SOUND_THREADS 4


namespace Anki {
  namespace Cozmo {
    
    namespace {
      
      int _numActiveThreads;
      
      bool _hasCmdProcessor;
      bool _hasRootDir;
      
      // Root directory of sounds
      const s32 MAX_PATH_LENGTH = 512;
      char _rootDir[MAX_PATH_LENGTH];
      
      SoundSchemeID_t _currScheme = SOUND_SCHEME_COZMO;
      
      // Table of sound files relative to root dir
      const std::map<SoundID_t, std::string> _soundTable[NUM_SOUND_SCHEMES] =
      {
        {
          // Cozmo default sound scheme
          {SOUND_TADA, ""}
          ,{SOUND_NOPROBLEMO, ""}
          ,{SOUND_INPUT, ""}
          ,{SOUND_SWEAR, ""}
          ,{SOUND_STARTOVER, "demo/WaitingForDice2.wav"}
          ,{SOUND_NOTIMPRESSED, "demo/OKGotIt.wav"}
          ,{SOUND_60PERCENT, ""}
          ,{SOUND_DROID, ""}
          
          ,{SOUND_DEMO_START, ""}
          ,{SOUND_WAITING4DICE, "demo/WaitingForDice1.wav"}
          ,{SOUND_WAITING4DICE2DISAPPEAR, "demo/WaitingForDice2.wav"}
          ,{SOUND_OK_GOT_IT, "demo/OKGotIt.wav"}
          ,{SOUND_OK_DONE, "demo/OKDone.wav"}
        }
        ,{
          // Movie sound scheme
          {SOUND_TADA, "misc/tada.mp3"}
          ,{SOUND_NOPROBLEMO, "misc/nproblem.wav"}
          ,{SOUND_INPUT, "misc/input.wav"}
          ,{SOUND_SWEAR, "misc/swear.wav"}
          ,{SOUND_STARTOVER, "anchorman/startover.wav"}
          ,{SOUND_NOTIMPRESSED, "anchorman/notimpressed.wav"}
          ,{SOUND_60PERCENT, "anchorman/60percent.wav"}
          ,{SOUND_DROID, "droid/droid.wav"}
          
          ,{SOUND_DEMO_START, "misc/swear.wav"}
          ,{SOUND_WAITING4DICE, "misc/input.wav"}
          ,{SOUND_WAITING4DICE2DISAPPEAR, "misc/input.wav"}
          ,{SOUND_OK_GOT_IT, "misc/nproblem.wav"}
          ,{SOUND_OK_DONE, "anchorman/60percent.wav"}
        }
      };
      
    }
    
    SoundManager* SoundManager::singletonInstance_ = nullptr;
    
    void SoundManager::removeInstance()
    {
      // check if the instance has been created yet
      if(nullptr != singletonInstance_) {
        delete singletonInstance_;
        singletonInstance_ = nullptr;
      }
    }
    
    SoundManager::SoundManager()
    {
      _numActiveThreads = 0;
      _hasCmdProcessor = false;
      _hasRootDir = false;
      
      if (system(NULL)) {
        _hasCmdProcessor = true;
        SetRootDir("basestation/cozmo_sounds");
      } else {
        PRINT_NAMED_WARNING("SoundManager.NoCmdProc","\n");
      }
    }

    void CmdLinePlay(SoundID_t id, const u8 numLoops)
    {
      if( !_soundTable[_currScheme].at(id).empty() ) {
        u8 timesPlayed = 0;
        while(timesPlayed++ < numLoops) {
          char fullCmd[512];
          snprintf(fullCmd, 512, "afplay %s/%s", _rootDir, _soundTable[_currScheme].at(id).c_str());
          system(fullCmd);
        }
      }
      --_numActiveThreads;
    }

    
    bool SoundManager::SetRootDir(const char* dir)
    {
      _hasRootDir = false;
      
      std::string fullPath(PREPEND_SCOPED_PATH(PlatformPathManager::Resource, dir));
      
      // Check if directory exists
      struct stat info;
      if( stat( fullPath.c_str(), &info ) != 0 ) {
        PRINT_NAMED_WARNING("SoundManager.SetRootDir.NoAccess",
                            "Could not access path %s (errno %d)\n",
                            fullPath.c_str(), errno);
        return false;
      }
      if (!S_ISDIR(info.st_mode)) {
        PRINT_NAMED_WARNING("SoundManager.SetRootDir.NotADir", "\n");
        return false;
      }
      
      _hasRootDir = true;
      snprintf(_rootDir, MAX_PATH_LENGTH, "%s", fullPath.c_str());
      return true;
    }
    
    SoundID_t SoundManager::GetID(const std::string& name)
    {
      static const std::map<std::string, SoundID_t> LUT = {
        {"TADA",      SOUND_TADA},
        {"OK_GOT_IT", SOUND_OK_GOT_IT},
        {"OK_DONE",   SOUND_OK_DONE},
        {"STARTOVER", SOUND_STARTOVER}
      };

      auto result = LUT.find(name);
      if(result == LUT.end()) {
        PRINT_NAMED_WARNING("SoundManager.GetID.UnknownSound", "No sound named '%s', ignoring.\n");
        return NUM_SOUNDS;
      } else {
        return result->second;
      }
    }
    
    bool SoundManager::Play(const std::string& name, const u8 numLoops)
    {
      return Play(SoundManager::GetID(name), numLoops);
    }
    
    bool SoundManager::Play(const SoundID_t id, const u8 numLoops)
    {
      if (_hasCmdProcessor && _hasRootDir && id < NUM_SOUNDS) {
        if (_numActiveThreads < MAX_SOUND_THREADS) {
          ++_numActiveThreads;
          
          std::thread soundThread(CmdLinePlay, id, numLoops);
          soundThread.detach();
          return true;
        }
      }
      return false;
    }
    
    void SoundManager::SetScheme(const SoundSchemeID_t scheme)
    {
      _currScheme = scheme;
    }
    
    SoundSchemeID_t SoundManager::GetScheme() const
    {
      return _currScheme;
    }
  } // namespace Cozmo
} // namespace Anki
