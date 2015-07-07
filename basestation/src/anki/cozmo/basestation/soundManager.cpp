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

#include "anki/cozmo/basestation/soundManager.h"

#include "util/logging/logging.h"
#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/platformPathManager.h"

#include <thread>
#include <map>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEBUG_SOUND_MANAGER 0

namespace Anki {
  namespace Cozmo {
    
    namespace {
      // Putting these here (as opposed to a member in SoundManager) so that the
      // CmdLinePlayFeeder, which runs in its own threads, can access them
      bool         _running;
      bool         _stopCurrSound;
      std::string  _soundToPlay;
      s32          _numLoops;
      bool         _isLocked;
      f32          _volume;
      
      std::string GetSoundToPlay(s32& numLoops, f32& volume)
      {
        // Wait for unlock before reading sound file string
        while(_isLocked) {
          usleep(1000);
        }
        _isLocked = true;
        numLoops = _numLoops;
        std::string retVal(_soundToPlay);
        volume = _volume;
        _isLocked = false;
        
        return retVal;
      }
      
      void SetSoundToPlay(const std::string soundToPlay,
                          const s32 numLoops,
                          const u8  volume = 100)
      {
        _isLocked = true;
        _soundToPlay = soundToPlay;
        _numLoops = numLoops;
        _volume   = static_cast<f32>(volume) * .01f;
        _isLocked = false;
      }
      
      void CmdLinePlay(const std::string soundFile,
                       const u8 numLoops,
                       const f32 volume)
      {

        const std::string cmd("afplay -v " + std::to_string(volume) + " " + soundFile);
        std::string fullCmd(cmd);
        for(s32 iLoop=1; iLoop < numLoops; ++iLoop) {
          fullCmd += " && " + cmd;
        }
        
        // Important: relinquish control so as not to block (and allow us to
        // kill playing sounds)
        fullCmd += " &";
        
        // Play the commanded sound
#       if DEBUG_SOUND_MANAGER
        printf("CmdLinePlay: %s\n", fullCmd.c_str());
#       endif
        system(fullCmd.c_str());

      } // CmdLinePlay()
      
      inline void KillPlayingSounds()
      {
#       if DEBUG_SOUND_MANAGER
        printf("Killing afplay threads\n");
#       endif
        system("pkill -f afplay");
      }
      
      void CmdLinePlayFeeder()
      {
        PRINT_STREAM_INFO("CmdLinePlayFeeder", "Started Sound Feeder thread...");
        while (_running) {
          usleep(10000);
          
          if (_stopCurrSound) {
            KillPlayingSounds();
            _stopCurrSound = false;
          } else {
            s32 numLoops;
            f32 volume;
            std::string soundToPlay(GetSoundToPlay(numLoops, volume));
            
            if (!soundToPlay.empty()) {
              // Start sound thread
              KillPlayingSounds();
              std::thread soundThread(CmdLinePlay, soundToPlay, numLoops, volume);
              soundThread.detach();
              
              SetSoundToPlay("", 1);
            }
          }
        }
        
        // Don't leave any sounds running
        KillPlayingSounds();
        
        PRINT_STREAM_INFO("CmdLinePlayFeeder", "Terminated Sound Feeder thread");
      }
      
    } // anonymous namespace
    
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
      _numLoops = 1;
      _running = true;
      _isLocked = false;
      
      std::thread soundFeederThread(CmdLinePlayFeeder);
      soundFeederThread.detach();
      usleep(100000);
      
      if (system(NULL)) {
        _hasCmdProcessor = true;
        SetRootDir("");
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
      
      std::string fullPath(PREPEND_SCOPED_PATH(PlatformPathManager::Sound, dir));
      
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
      _rootDir = fullPath;
      
      return true;
      
    } // SetRootDir()
    
    const std::string& SoundManager::GetSoundFile(SoundID_t soundID) const
    {
      // Table of sound files relative to root dir
      static const std::map<SoundID_t, std::string> soundTable[NUM_SOUND_SCHEMES] =
      {
        {
          // Cozmo default sound scheme
          {SOUND_STARTOVER, "demo/WaitingForDice2.wav"}
          ,{SOUND_NOTIMPRESSED, "demo/OKGotIt.wav"}
          
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
        ,{
          // CREEP Sound Scheme
          // TODO: Update these mappings for real playtest sounds
          {SOUND_POWER_ON,     "creep/Robot-PowerOn1Rev2.mp3"}
          ,{SOUND_POWER_DOWN,  "creep/Robot-PowerDown13B.mp3"}
          ,{SOUND_PHEW,        "creep/Robot-ReliefPhew1.mp3"}
          ,{SOUND_SCREAM,      "creep/Robot-Scream7.mp3"}
          ,{SOUND_OOH,         "creep/Robot-OohScream12.mp3"}
          ,{SOUND_WHATS_NEXT,  "creep/Robot-WhatsNext1A.mp3"}
          ,{SOUND_HELP_ME,     "creep/Robot-WahHelpMe2.mp3"}
          ,{SOUND_SCAN,        "creep/Robot-Scanning2Rev1.mp3"}
          ,{SOUND_HAPPY_CHASE, "creep/Robot-Happy2.mp3"}
          ,{SOUND_FLEES,       "creep/Robot-Happy1.mp3"}
          ,{SOUND_SINGING,     "creep/Robot-Singing1Part1-2.mp3"}
          ,{SOUND_SHOOT,       "codeMonsterShooter/shoot.wav"}
          ,{SOUND_SHOT_HIT,    "codeMonsterShooter/hit.wav"}
          ,{SOUND_SHOT_MISSED, "codeMonsterShooter/miss.wav"}
          
        }
      };
      
      static const std::string DEFAULT("");
      
      auto result = soundTable[_currScheme].find(soundID);
      if(result == soundTable[_currScheme].end()) {
        PRINT_NAMED_WARNING("SoundManager.GetSoundFile.UndefinedID",
                            "No file defined for sound ID %d in sound scheme %d.\n",
                            soundID, _currScheme);
        return DEFAULT;
      } else {
        return result->second;
      }
      
    } // GetSoundFile()
    
    
    SoundID_t SoundManager::GetID(const std::string& name)
    {
      static const std::map<std::string, SoundID_t> LUT = {
        {"DROID",         SOUND_DROID},
        {"FLEES",         SOUND_FLEES},
        {"NOPROBLEMO",    SOUND_NOPROBLEMO},
        {"NOTIMPRESSED",  SOUND_NOTIMPRESSED},
        {"OK_DONE",       SOUND_OK_DONE},
        {"OK_GOT_IT",     SOUND_OK_GOT_IT},
        {"OOH",           SOUND_OOH},
        {"PHEW",          SOUND_PHEW},
        {"POWER_DOWN",    SOUND_POWER_DOWN},
        {"POWER_ON",      SOUND_POWER_ON},
        {"HAPPY_CHASE",   SOUND_HAPPY_CHASE},
        {"HELP_ME",       SOUND_HELP_ME},
        {"INPUT",         SOUND_INPUT},
        {"SCAN",          SOUND_SCAN},
        {"SCREAM",        SOUND_SCREAM},
        {"SHOT_HIT",      SOUND_SHOT_HIT},
        {"SHOT_MISSED",   SOUND_SHOT_MISSED},
        {"SHOOT",         SOUND_SHOOT},
        {"SINGING",       SOUND_SINGING},
        {"STARTOVER",     SOUND_STARTOVER},
        {"SWEAR",         SOUND_SWEAR},
        {"TADA",          SOUND_TADA},
        {"WHATS_NEXT",    SOUND_WHATS_NEXT},
      };

      auto result = LUT.find(name);
      if(result == LUT.end()) {
        PRINT_STREAM_WARNING("SoundManager.GetID.UnknownSound", "No sound named '" << name << "', ignoring.");
        return NUM_SOUNDS;
      } else {
        return result->second;
      }
    } // GetID()
    
    bool SoundManager::Play(const std::string& name, const u8 numLoops, const u8 volume)
    {
      return Play(SoundManager::GetID(name), numLoops, volume);
    }
    
    bool SoundManager::Play(const SoundID_t id, const u8 numLoops, const u8 volume)
    {
      if (_hasCmdProcessor && _hasRootDir && id < NUM_SOUNDS) {
        const std::string& soundFile = GetSoundFile(id);
        if( !soundFile.empty() )
        {
          SetSoundToPlay(_rootDir + "/" + soundFile, numLoops, volume);
          return true;
        }
      }
      return false;
    }

    void SoundManager::Stop()
    {
      SetSoundToPlay("", 1);
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
    
    std::string GetStdoutFromCommand(std::string cmd)
    {
      std::string data;
      FILE * stream;
      const int max_buffer = 256;
      char buffer[max_buffer];
      cmd.append(" 2>&1");
      
      stream = popen(cmd.c_str(), "r");
      if (stream) {
        while (!feof(stream))
          if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
      }
      return data;
    }
    
    const u32 SoundManager::GetSoundDurationInMilliseconds(SoundID_t soundID) const
    {
      const std::string soundFile = GetSoundFile(soundID);
      
      // This is disgusting.
      // grep the result of afinfo to parse out the sound duration
      const std::string cmd("afinfo -b -r " + _rootDir + soundFile +
                            " | grep -o '[0-9]\\{1,\\}\\.[0-9]\\{1,\\}'");
      
#     if DEBUG_SOUND_MANAGER
      printf("GetSoundDurationInMilliseconds command: %s\n", cmd.c_str());
#     endif
      
      std::string output = GetStdoutFromCommand(cmd);
      
      u32 duration_ms = 0;
      
      if(output.empty()) {
        PRINT_NAMED_WARNING("SoundManager.GetSoundDurationInMilliseconds",
                            "Failed to get duration string for soundID=%d, file %s.\n",
                            soundID, soundFile.c_str());
      } else {
        const float duration_sec = std::stof(output);
        duration_ms = static_cast<u32>(duration_sec * 1000);
      }
      
      return duration_ms;
    }
    

    
  } // namespace Cozmo
} // namespace Anki
