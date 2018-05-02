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

#include <sodium.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <vector>

#pragma once

namespace Anki {
namespace Switchboard {

struct __attribute__ ((packed)) RtsIdData {
  bool hasName;
  char name[12];
  uint8_t publicKey[crypto_kx_PUBLICKEYBYTES];
  uint8_t privateKey[crypto_kx_SECRETKEYBYTES];
};

struct __attribute__ ((packed)) RtsClientData {
  uint8_t publicKey[crypto_kx_PUBLICKEYBYTES];
  uint8_t sessionRx[crypto_kx_SESSIONKEYBYTES];
  uint8_t sessionTx[crypto_kx_SESSIONKEYBYTES];
};

struct __attribute__ ((packed)) RtsKeysData {
  uint8_t magic[8];
  uint32_t version;
  RtsIdData id;
  uint8_t numKnownClients;
};

struct RtsKeys {
  RtsKeysData keys;
  std::vector<RtsClientData> clients;
};

class SavedSessionManager {
public:
  static RtsKeys LoadRtsKeys();
  static void SaveRtsKeys(RtsKeys& keys);
  static const std::string kRtsKeyPath;

private:
  static bool MakeDirectory(std::string directory);
  
  static const std::string kSaveFolder;
  static const std::ios_base::openmode kWriteMode;
  static const std::ios_base::openmode kReadMode;
  static const uint8_t kMaxNumberClients;
  static const char* kPrefix;
  static const uint32_t kNativeBufferSize;
};

} // Switchboard
} // Anki
