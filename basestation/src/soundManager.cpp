/**
 * File: soundManager.cpp
 *
 * Author: Kevin Yoon
 * Date:   6/18/2014
 *
 * Description: Simple sound player, that only plays one sound at a time and only works on mac.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "soundManager.h"
#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/common/basestation/exceptions.h"

#include <thread>
#include <map>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


namespace Anki {
  namespace Cozmo {
    
    namespace {
      
      bool _running;
      bool _stopCurrSound;
      int _soundToPlay;
      
      
      bool _hasCmdProcessor;
      bool _hasRootDir;
      
      // Root directory of sounds
      char _rootDir[512];
      
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
    
    
    void CmdLinePlay(SoundID_t id)
    {
      if( !_soundTable[_currScheme].at(id).empty() ) {
        char fullCmd[512];
        
        // Kill whichever sound might currently be playing
        system("pkill -f afplay");
        
        // Play the commanded sound
        sprintf(fullCmd, "afplay %s/%s &", _rootDir, _soundTable[_currScheme].at(id).c_str());
        printf("CmdPlay %d\n", id);
        system(fullCmd);
      }
    }

    
    void CmdLinePlayFeeder()
    {
      PRINT_INFO("Started Sound Feeder thread...\n");
      while (_running) {
        usleep(10000);

        if (_stopCurrSound) {
          system("pkill -f afplay");
          _stopCurrSound = false;
        } else if (_soundToPlay >= 0) {
          // Start sound thread
          SoundID_t id = (SoundID_t)_soundToPlay;
          _soundToPlay = -1;
          std::thread soundThread(CmdLinePlay, id);
          soundThread.detach();
        }
      }
      PRINT_INFO("Terminated Sound Feeder thread\n");
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
      _hasCmdProcessor = false;
      _hasRootDir = false;
      
      _stopCurrSound = false;
      _running = true;
      std::thread soundFeederThread(CmdLinePlayFeeder);
      soundFeederThread.detach();
              usleep(100000);
      
      if (system(NULL)) {
        _hasCmdProcessor = true;
        SetRootDir("cozmo_sounds");
      } else {
        PRINT_NAMED_WARNING("SoundManager.NoCmdProc","\n");
      }
    }

    SoundManager::~SoundManager()
    {
      _running = false;
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
    
    
    bool SoundManager::Play(const SoundID_t id)
    {
      if (_hasCmdProcessor && _hasRootDir && id < NUM_SOUNDS) {
        _soundToPlay = id;
        return true;
      }
      return false;
    }

    void SoundManager::Stop()
    {
      _soundToPlay = -1;
      _stopCurrSound = true;
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
