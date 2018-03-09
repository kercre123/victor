/**
 * File: savedSessionManager.h
 *
 * Author: paluri
 * Created: 3/7/2018
 *
 * Description: Saved and load public key / session key information
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <stdlib.h>
#include <fstream>

#ifndef SavedSessionManager_h
#define SavedSessionManager_h

namespace Anki {
namespace Switchboard {

class SavedSessionManager {
public:
  static void SavePublicKey(const uint8_t* publicKey);
  static uint32_t LoadPublicKey(uint8_t* publicKeyOut);

  static void SaveSession(const uint8_t* clientPublicKey, const uint8_t* sessionKeyEncrypt, const uint8_t* sessionKeyDecrypt); 
  static uint32_t LoadSession(uint8_t* clientPublicKeyOut, uint8_t* sessionKeyEncryptOut, uint8_t* sessionKeyDecryptOut);

private:
  static int MakeDirectory();
  
  static const std::string kSaveFolder;
  static const std::string kPublicKeyPath;
  static const std::string kKnownSessionsPath;
  static const std::ios_base::openmode kWriteMode;
  static const std::ios_base::openmode kReadMode;
  static const char* kPrefix;
};

} // Switchboard
} // Anki

#endif
