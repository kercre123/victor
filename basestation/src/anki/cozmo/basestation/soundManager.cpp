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
#include "anki/cozmo/basestation/keyframe.h"
#include "util/logging/logging.h"
#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/platformPathManager.h"
#include "adpcm.h"

#include <thread>
#include <map>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

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
      
      _currOpenSoundFileName = "";
      _currOpenSoundFilePtr = nullptr;
      _currOpenSoundNumSamples = 0;
      
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
    
    
    bool SoundManager::SetRootDir(const std::string dir)
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
      
      ReadSoundDir("robot", true);
      ReadSoundDir("device", false);
      
      return true;
      
    } // SetRootDir()
    
    // Helper for getting result back from calling a system command:
    static std::string GetStdoutFromCommand(std::string cmd)
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
    
    
    static s32 GetAudioDurationInMilliseconds(std::string fileName) {
      // This is disgusting.
      // grep the result of afinfo to parse out the sound duration
      // TODO: Do something that works on iOS!
      const std::string cmd("afinfo -b -r " + fileName +
                            " | grep -o '[0-9]\\{1,\\}\\.[0-9]\\{1,\\}'");
      
      std::string output = GetStdoutFromCommand(cmd);
      
      #if DEBUG_SOUND_MANAGER
      printf("SoundManager:: Sound duration command: %s\n", cmd.c_str());
      #endif
      
      if(output.empty()) {
        return -1;
      }
      
      const float duration_sec = std::stof(output);
      return static_cast<u32>(duration_sec * 1000);
    }

    static bool IsValidRobotAudio(std::string fileName) {
      const std::string cmd("afinfo -b " + fileName + " | grep 'format:'");
      std::string output = GetStdoutFromCommand(cmd);
      
      bool validNumChannels = (output.find(" 1 ch,") != std::string::npos);
      bool validFreq = (output.find(" 24000 Hz,") != std::string::npos);
      bool validEncoding = (output.find("\'lpcm\'") != std::string::npos) && (output.find(" 16-bit little-endian signed integer") != std::string::npos);
      
      bool isValid = (validNumChannels && validFreq && validEncoding);
      
      #if DEBUG_SOUND_MANAGER
      if (!isValid) {
        PRINT_NAMED_WARNING("SoundManager.IsValidRobotAudio.IsInvalid",
                            "Sound %s is invalid for robot audio. (%s)(%d %d %d)\n",
                            fileName.c_str(), output.c_str(), validNumChannels, validFreq, validEncoding);
      }
      #endif
    
      return isValid;
    }
    
    
    Result SoundManager::ReadSoundDir(std::string subDir, bool isRobotAudio)
    {
      if(!subDir.empty()) {
        subDir += "/";
      }
      
      DIR* dir = opendir((_rootDir + subDir).c_str());
      
      if ( dir != nullptr) {
        dirent* ent = nullptr;
        while ( (ent = readdir(dir)) != nullptr) {
          if (ent->d_type == DT_REG && ent->d_name[0] != '.') {
            const std::string soundName(subDir + ent->d_name);
            std::string fullSoundFilename = _rootDir + subDir + ent->d_name;
            struct stat attrib{0};
            int result = stat(fullSoundFilename.c_str(), &attrib);
            if (result == -1) {
              PRINT_NAMED_WARNING("SoundManager.ReadSoundDir",
                                  "could not get mtime for %s", fullSoundFilename.c_str());
              continue;
            }
            bool loadSoundFile = false;
            auto mapIt = _availableSounds.find(soundName);
            if (mapIt == _availableSounds.end()) {
              _availableSounds[soundName].lastLoadedTime = attrib.st_mtimespec.tv_sec;
              loadSoundFile = true;
            } else {
              if (mapIt->second.lastLoadedTime < attrib.st_mtimespec.tv_sec) {
                mapIt->second.lastLoadedTime = attrib.st_mtimespec.tv_sec;
                loadSoundFile = true;
              } else {
                //PRINT_NAMED_INFO("Robot.ReadAnimationFile", "old time stamp for %s", fullFileName.c_str());
              }
            }
            if (loadSoundFile) {
              
              // Compute the sound's duration:
              s32 duration_ms = GetAudioDurationInMilliseconds(fullSoundFilename);
              
              if (duration_ms < 0) {
                PRINT_NAMED_WARNING("SoundManager.ReadSoundDir",
                                    "Failed to get duration string for '%s', file %s.\n",
                                    soundName.c_str(), fullSoundFilename.c_str());
              }
            
              AvailableSound& availableSound = _availableSounds[soundName];
              availableSound.duration_ms = duration_ms;
              availableSound.fullFilename = fullSoundFilename;
              
              // Add to availble robot sound if it has proper encoding
              if (isRobotAudio) {
                if (IsValidRobotAudio(fullSoundFilename)) {
                  printf("FOUND ROBOT AUDIO %s\n", soundName.c_str());
                  _availableRobotSounds[soundName] = _availableSounds[soundName];
                } else {
                  PRINT_NAMED_WARNING("SoundManager.ReadSoundDir.InvalidRobotAudio",
                                      "Sound %s is invalid for robot audio.\n",
                                      soundName.c_str());
                }
              }
              
              
#             if DEBUG_SOUND_MANAGER
              PRINT_NAMED_INFO("SoundManager.ReadSoundDir",
                               "Added %dms sound '%s' in file '%s'\n",
                               availableSound.duration_ms,
                               soundName.c_str(),
                               availableSound.fullFilename.c_str());
#             endif
              
            }
          } // if (ent->d_type == DT_REG) {
          else if(ent->d_type == DT_DIR && ent->d_name[0] != '.') {
            ReadSoundDir(subDir + ent->d_name, isRobotAudio);
          }
        }
        closedir(dir);
        
      } else {
        PRINT_NAMED_ERROR("SoundManager.ReadSoundDir",
                          "Sound folder not found: %s\n",
                          _rootDir.c_str());
        return RESULT_FAIL;
      }

      if(subDir.empty()) { // Only display this message at the root
        PRINT_NAMED_INFO("SoundManager.ReadSoundDir",
                         "SoundManager now contains %lu available sounds.\n",
                         _availableSounds.size());
      }
      
      return RESULT_OK;
    }
    
    bool SoundManager::Play(const std::string& name, const u8 numLoops, const u8 volume)
    {
      auto soundIter = _availableSounds.find(name);
      
      if (_hasCmdProcessor && _hasRootDir && soundIter != _availableSounds.end()) {
        SetSoundToPlay(soundIter->second.fullFilename, numLoops, volume);
        return true;
      }
      return false;
    }

    void SoundManager::Stop()
    {
      SetSoundToPlay("", 1);
      _stopCurrSound = true;
    }
    
    
    bool SoundManager::IsValidSound(const std::string& name) const
    {
      return _availableSounds.find(name) != _availableSounds.end();
    }
    
    const u32 SoundManager::GetSoundDurationInMilliseconds(const std::string& name) const
    {
      auto soundIter = _availableSounds.find(name);
      if(soundIter == _availableSounds.end()) {
        PRINT_NAMED_ERROR("SoundManager.GetSoundDurationInMilliseconds", "No sound named '%s'\n", name.c_str());
        return 0;
      }
      
      return soundIter->second.duration_ms;
    }

    bool SoundManager::GetSoundSample(const std::string& name, const u32 sampleIdx, MessageAnimKeyFrame_AudioSample &msg)
    {
      if (_currOpenSoundFileName != name) {
        // Dump file contents to buffer if this is not the same file
        // from which a sound sample was last requested.
        
        auto soundIt = _availableRobotSounds.find(name);
        if (soundIt == _availableRobotSounds.end()) {
          PRINT_NAMED_WARNING("SoundManager.GetSoundSample.SoundNotAvailable", "Name: %s", name.c_str());
          return nullptr;
        }
        
        // Close file if one was already open
        if (_currOpenSoundFilePtr != nullptr) {
          fclose(_currOpenSoundFilePtr);
          _currOpenSoundFilePtr = nullptr;
        }
        
        // Open sound file
        _currOpenSoundFilePtr = fopen(soundIt->second.fullFilename.c_str(), "rb");
        if (_currOpenSoundFilePtr==nullptr) {
          PRINT_NAMED_WARNING("SoundManager.GetSoundSample.FileOpenFail", "%s", soundIt->second.fullFilename.c_str());
          return nullptr;
        }
        
        // obtain file size:
        fseek (_currOpenSoundFilePtr , 0 , SEEK_END);
        long int fileSize = ftell (_currOpenSoundFilePtr);
        rewind (_currOpenSoundFilePtr);
        
        // TODO: Strip off wav file header
        fseek(_currOpenSoundFilePtr, 44, SEEK_SET);
        fileSize -= 44;
        
        // Read file contents to buffer
        fileSize = MIN(fileSize, MAX_SOUND_BUFFER_SIZE);
        fread(_soundBuf, 1, fileSize, _currOpenSoundFilePtr);
        
        _currOpenSoundNumSamples = fileSize / UNENCODED_SOUND_SAMPLE_SIZE;
        
        PRINT_NAMED_INFO("SoundManager.GetSoundSample.Info","Opening %s - duration %f s\n", name.c_str(), _currOpenSoundNumSamples * RobotAudioKeyFrame::SAMPLE_LENGTH_MS * 0.001);

        // Check that the number of samples doesn't exceed the buffer
        if (_currOpenSoundNumSamples > MAX_SOUND_BUFFER_SIZE / UNENCODED_SOUND_SAMPLE_SIZE) {
          _currOpenSoundNumSamples = MAX_SOUND_BUFFER_SIZE / UNENCODED_SOUND_SAMPLE_SIZE;
        }
        
        _currOpenSoundFileName = name;
        _adpcm_index = 0;
        _adpcm_predictor = 0;
      }
      
      if (sampleIdx >= _currOpenSoundNumSamples) {
        return false;
      }

      msg.predictor = _adpcm_predictor;
      msg.index = _adpcm_index;
      
      s16* frame = &(_soundBuf[sampleIdx * SOUND_SAMPLE_SIZE * 2]);
      encodeADPCM(_adpcm_index, _adpcm_predictor, frame, msg.sample.data(), SOUND_SAMPLE_SIZE * 2);
      
      /*
       // Debug dump first 20 samples
      if (sampleIdx < 20) {
        FILE* f = fopen("encoded.adp", "a");
        fwrite(msg.sample.data(), 1, 400, f);
        fwrite(&msg.predictor, 1, 2, f);
        fwrite(&msg.index, 1, 1, f);
        fclose(f);
      }
       */
      
      return true;
    }
    
  } // namespace Cozmo
} // namespace Anki
