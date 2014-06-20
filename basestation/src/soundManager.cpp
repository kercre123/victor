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
#include "anki/common/basestation/general.h"
#include "anki/common/basestation/exceptions.h"

#include <thread>
#include <map>

#include <sys/types.h>
#include <sys/stat.h>

#define MAX_SOUND_THREADS 2


namespace Anki {
  namespace Cozmo {
    
    namespace {
      
      int _numActiveThreads;
      
      bool _hasCmdProcessor;
      bool _hasRootDir;
      
      // Root directory of sounds
      char _rootDir[512];
      
      // Table of sound files relative to root dir
      const std::map<SoundID_t, std::string> _soundTable =
      {
        {SOUND_TADA, "misc/tada.mp3"}
        ,{SOUND_NOPROBLEMO, "misc/nproblem.wav"}
        ,{SOUND_INPUT, "misc/input.wav"}
        ,{SOUND_SWEAR, "misc/swear.wav"}
        ,{SOUND_STARTOVER, "anchorman/startover.wav"}
        ,{SOUND_NOTIMPRESSED, "anchorman/notimpressed.wav"}
        ,{SOUND_60PERCENT, "anchorman/60percent.wav"}
        ,{SOUND_DROID, "droid/droid.wav"}
      };
      
    }
    
    SoundManager* SoundManager::singletonInstance_ = 0;
    
    SoundManager::SoundManager()
    {
      _numActiveThreads = 0;
      _hasCmdProcessor = false;
      _hasRootDir = false;
      
      if (system(NULL)) {
        _hasCmdProcessor = true;
        SetRootDir("cozmo_sounds");
      } else {
        PRINT_NAMED_WARNING("SoundManager.NoCmdProc","\n");
      }
    }

    void CmdLinePlay(SoundID_t id)
    {
      char fullCmd[512];
      sprintf(fullCmd, "afplay %s/%s", _rootDir, _soundTable.at(id).c_str());
      system(fullCmd);
      --_numActiveThreads;
    }

    
    bool SoundManager::SetRootDir(const char* dir)
    {
      _hasRootDir = false;
      
      // Check if directory exists
      struct stat info;
      if( stat( dir, &info ) != 0 ) {
        PRINT_NAMED_WARNING("SoundManager.SetRootDir.NoAccess", "Could not access path %s (errno %d)\n", dir, errno);
        return false;
      }
      if (!S_ISDIR(info.st_mode)) {
        PRINT_NAMED_WARNING("SoundManager.SetRootDir.NotADir", "\n");
        return false;
      }
      
      _hasRootDir = true;
      sprintf(_rootDir, "%s", dir);
      return true;
    }
    
    
    bool SoundManager::Play(SoundID_t id)
    {
      if (_hasCmdProcessor && _hasRootDir) {
        if (_numActiveThreads < MAX_SOUND_THREADS) {
          ++_numActiveThreads;
          std::thread soundThread(CmdLinePlay, id);
          soundThread.detach();
          return true;
        }
      }
      return false;
    }
    
    
  } // namespace Cozmo
} // namespace Anki
