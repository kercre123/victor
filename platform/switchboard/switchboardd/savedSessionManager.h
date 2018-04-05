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

#pragma once

namespace Anki {
namespace Switchboard {

class SavedSessionManager {
public:
  static void SaveKey(const uint8_t* key, size_t size, const std::string& path);
  static uint32_t LoadKey(uint8_t* keyOut, size_t size, const std::string& path);

  static void SaveSession(const uint8_t* clientPublicKey, const uint8_t* sessionKeyEncrypt, const uint8_t* sessionKeyDecrypt); 
  static uint32_t LoadSession(uint8_t* clientPublicKeyOut, uint8_t* sessionKeyEncryptOut, uint8_t* sessionKeyDecryptOut);

  static const std::string kPublicKeyPath;
  static const std::string kPrivateKeyPath;

private:
  static bool MakeDirectory();
  
  static const std::string kSaveFolder;
  static const std::string kKnownSessionsPath;
  static const std::ios_base::openmode kWriteMode;
  static const std::ios_base::openmode kReadMode;
  static const char* kPrefix;
};

} // Switchboard
} // Anki
