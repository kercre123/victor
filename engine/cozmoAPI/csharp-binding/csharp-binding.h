//
//  anki/cozmo/csharp-binding/csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 4/11/15.
//
//

//
// This file is not used on victor. Contents are preserved for reference.
//
#if !defined(VICOS_LA) && !defined(VICOS_LE)

#ifndef __CSharpBinding__csharp_binding__
#define __CSharpBinding__csharp_binding__

#include <string>

#ifndef _cplusplus
extern "C" {
#endif

  // Hook for initialization code. If this errors out, do not run anything else.
  int cozmo_startup(const char *configuration_data);

  // Hook for deinitialization. Should be fine to call startup after this call, even on failure.
  // Return value is just for informational purposes. Should never fail, even if not initialized.
  int cozmo_shutdown();

  // Called from C# to get messages from the engine
  size_t cozmo_transmit_engine_to_game(uint8_t* buffer, size_t size);

  // Called from C# to push messages to the engine
  void cozmo_transmit_game_to_engine(const uint8_t* buffer, size_t size);

  // Invoked by background OS notifications when we have internet
  void cozmo_execute_background_transfers();

  // Hook for triggering setup of the desired wifi details
  int cozmo_wifi_setup(const char* wifiSSID, const char* wifiPasskey);

  // Install/uninstall Google Breakpad for Android C++ crash reporting
  // (no-op on other platforms)
  void cozmo_install_google_breakpad(const char* path);
  void cozmo_uninstall_google_breakpad();

  void cozmo_send_to_clipboard(const char* log);

  uint32_t cozmo_activate_experiment(const uint8_t* requestBuffer, size_t requestSize,
                                     uint8_t* responseBuffer, size_t responseSize);

  // Called from C# to locate device ID file. Android only.
  const char * cozmo_get_device_id_file_path(const char * persistentDataPath);

  void Unity_DAS_Event(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount);

  void Unity_DAS_LogE(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount);

  void Unity_DAS_LogW(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount);

  void Unity_DAS_LogI(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount);

  void Unity_DAS_LogD(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount);

  void Unity_DAS_Ch_LogI(const char* channelName, const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount);

  void Unity_DAS_Ch_LogD(const char* channelName, const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount);

  void Unity_DAS_SetGlobal(const char* key, const char* value);

#ifndef _cplusplus
}
#endif

#endif // __CSharpBinding__csharp_binding__

#endif
