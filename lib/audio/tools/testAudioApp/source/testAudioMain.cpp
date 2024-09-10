/**
 * File: testAudioMain.cpp
 *
 * Author: Jordan Rivas
 * Created: 1/16/18
 *
 * Description: This is a standalone command line app to test Anki's Audio Library. See README.md for details
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "testAudioLogger.h"
#include "testAudioEngineController.h"
#include <unistd.h>


void PrintHelp() {
  printf("\n********************************************************************************\n");
  printf("Anki Test Audio App HELP\n");
  printf("********************************************************************************\n");
  printf("-h          Print this message\n");
  printf("-a          Asset Path Directory, Wwise Generated Sound Banks **Required**\n");
  printf("-s          Audio Scene File Path **Required**\n");
  printf("-l          Locale title (Default is 'EnglishUS')\n");
  printf("-r          Repeat Count\n");
  printf("-d          Delay in seconds between Audio Events (Default is 1.0 Sec)\n");
  printf("-w          File path to write Session logs to (Default logs are OFF)\n");
  printf("-v          Number of voices (a.k.a simultaneously events) (Default is 1)\n");
  printf("-D          Enable Debug Logs\n");
  printf("********************************************************************************\n");
}

int main(int argc, const char * argv[]) {
  
  LOG_INFO("Hello, Test Audio World!");
  
  std::string assetPath;
  std::string sceneFilePath;
  std::string localeTitle;
  std::string sessionLogFilePath;
  int repeatCount = 0;
  float delaySec = 1.0f;
  bool sessionLoggingOn = false;
  int voiceCount = 1;
  for (size_t idx = 0; idx < argc; ++idx) {
    const char* argVal = argv[idx];
    if (std::strcmp(argVal, "-h") == 0) {
      // Help
      PrintHelp();
      return 0;
    }
    else if (std::strcmp(argVal, "-a") == 0) {
      // Sound Bank Asset path
      if (idx + 1 < argc) {
        assetPath  = argv[idx + 1];
      }
      else {
        LOG_ERROR("-a Not enough arguments!");
        return 1;
      }
    }
    else if (std::strcmp(argVal, "-s") == 0) {
      // Audio Scene file path
      if (idx + 1 < argc) {
        sceneFilePath  = argv[idx + 1];
      }
      else {
        LOG_ERROR("-s Not enough arguments!");
        return 1;
      }
    }
    else if (std::strcmp(argVal, "-l") == 0) {
      // Locale title
      if (idx + 1 < argc) {
        localeTitle = argv[idx + 1];
      }
      else {
        LOG_ERROR("-l Not enough arguments!");
        return 1;
      }
    }
    else if (std::strcmp(argVal, "-r") == 0) {
      // Repeat count
      if (idx + 1 < argc) {
        repeatCount = std::stoi(std::string(argv[idx + 1]));
      }
      else {
        LOG_ERROR("-r Not enough arguments!");
        return 1;
      }
    }
    else if (std::strcmp(argVal, "-d") == 0) {
      // Delay between events
      if (idx + 1 < argc) {
        delaySec = std::stof(std::string(argv[idx + 1]));
        if (delaySec <= 0.0f) {
          LOG_ERROR("Delay must be > 0.0");
          return 1;
        }
      }
      else {
        LOG_ERROR("-d Not enough arguments!");
        return 1;
      }
    }
    else if (std::strcmp(argVal, "-w") == 0) {
      // Turn on session logs
      if (idx + 1 < argc) {
        sessionLoggingOn = true;
        sessionLogFilePath = argv[idx + 1];
      }
      else {
        LOG_ERROR("-w Not enough arguments!");
        return 1;
      }
    }
    else if (std::strcmp(argVal, "-v") == 0) {
      // Number of voices
      if (idx + 1 < argc) {
        voiceCount = std::stof(std::string(argv[idx + 1]));
        if (voiceCount < 1) {
          LOG_ERROR("Voice Count must be > 0");
          return 1;
        }
      }
      else {
        LOG_ERROR("-v Not enough arguments!");
        return 1;
      }
    }
    else if (std::strcmp(argVal, "-D") == 0) {
      // Turn on debug logs
      TestAudioLogger::EnableDebugLogs = true;
      LOG_DEBUG("Enable Debug Logs");
    }
  }
 
  if (assetPath.empty() || sceneFilePath.empty()) {
    PrintHelp();
    LOG_ERROR("Invalid argumentes assetPath: '%s'  sceneFilePath: '%s'", assetPath.c_str(), sceneFilePath.c_str());
    return 1;
  }
  
  LOG_DEBUG("App Settings");
  LOG_DEBUG("Asset Path '%s'", assetPath.c_str());
  LOG_DEBUG("Audio Scene File Path '%s'", sceneFilePath.c_str());
  LOG_DEBUG("Repeat Count %d", repeatCount);
  LOG_DEBUG("Delay in Seconds %f", delaySec);
  LOG_DEBUG("Session Logging is %s", sessionLoggingOn ? "ON" : "OFF");
  if (sessionLoggingOn) {
    LOG_DEBUG("Session Log File Path '%s'", sessionLogFilePath.c_str());
  }
  
  // Start audio engine
  TestAudioEngineController audioCtr(assetPath, sessionLogFilePath, localeTitle);

  if (sessionLoggingOn) {
    // Start Session Capture
    audioCtr.WriteProfilerCapture(true, "ProfilerSession.prof");
    audioCtr.WriteAudioOutputCapture(true, "OutputSession.wav");
  }

  LOG_DEBUG("Voice Count %d", voiceCount);
  
  // Pause for a sec to start session
  usleep(1e6);
  
  // Load audio scene
  if (!audioCtr.LoadAudioScene(sceneFilePath)) {
    return 1;
  }

  // Pause for a sec to load audio scene
  usleep(1e6);
  
  // App vars
  const int fps = 30; // 30 frames per sec
  const uint32_t sleep_us = 1e6 / fps;
  float waitTime_s = 0.0f;
  uint32_t delayTickCount = 0;
  int activeVoices = 0;
  
  // Run Loop
  while (true) {
    // Play event
    if (activeVoices < voiceCount) {
      // Wait until it's time for the next event
      if (waitTime_s <= 0.0f) {
        // Play event
        if (!audioCtr.AllActionsCompleted()) {
          // Reset
          while ((activeVoices < voiceCount) && !audioCtr.AllActionsCompleted()) {
            ++activeVoices;
            LOG_DEBUG("audioCtr.PostNextAction Start | activeVoices: %d", activeVoices);
            delayTickCount = 0;
            // Perform Audio Action
            audioCtr.PostNextAction([&activeVoices](){
              --activeVoices;
              LOG_DEBUG("audioCtr.PostNextAction Complete | activeVoices: %d", activeVoices);
            });
          }
        }
        // All actions have been completed
        else if (activeVoices <= 0) {
          // Handle repeat
          --repeatCount;
          activeVoices = 0;
          LOG_DEBUG("--repeatCount => %d", repeatCount);
          audioCtr.RestActionIdx();
          if (repeatCount < 0) {
            LOG_DEBUG("Break out of run loop");
            break;
          }
        }
      }
      
      // Update wait time
      {
        const float fps_f = fps;
        waitTime_s = delaySec - (delayTickCount * (1.f / fps_f));
        ++delayTickCount;
        // LOG_DEBUG("WaitTime_s %f", waitTime_s);
      }
    }
    audioCtr.Update();
    // LOG_DEBUG("Sleep");
    usleep(sleep_us);
  }
  
  if (sessionLoggingOn) {
    // Close Session Capture
    audioCtr.WriteProfilerCapture(false);
    audioCtr.WriteAudioOutputCapture(false);
  }
  
  LOG_INFO("Goodbye, Test Audio World!");
  
  return 0;
}
