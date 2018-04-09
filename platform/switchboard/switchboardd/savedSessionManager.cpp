/**
 * File: savedSessionManager.cpp
 *
 * Author: paluri
 * Created: 3/7/2018
 *
 * Description: Saved and load public key / session key information
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "savedSessionManager.h"
#include <sodium.h>
#include <sys/stat.h>
#include "log.h"
#include "switchboardd/pairingMessages.h"

namespace Anki {
namespace Switchboard {

const std::string SavedSessionManager::kSaveFolder = "/data/switchboard";
const std::string SavedSessionManager::kPublicKeyPath = "id_rts.pub";
const std::string SavedSessionManager::kPrivateKeyPath = "id_rts";
const std::string SavedSessionManager::kKnownSessionsPath = "known_clients";
const std::ios_base::openmode SavedSessionManager::kWriteMode = std::ios_base::binary | std::ios_base::trunc;
const std::ios_base::openmode SavedSessionManager::kReadMode = std::ios_base::binary | std::ios_base::in;
const char* SavedSessionManager::kPrefix = "ANKIBITS";

void SavedSessionManager::SaveKey(const uint8_t* key, size_t size, const std::string& path) {
  std::ofstream fout;

  // Make Directory if not exist
  bool makeDirSuccess = MakeDirectory();

  if(!makeDirSuccess) {
    Log::Error("Could not successfully make directory.");
  }

  fout.open(kSaveFolder + "/" + path + ".tmp", kWriteMode);

  if(!fout.is_open()) {
    Log::Error("Could not open file.");
    return;
  }

  uint32_t version = SB_PAIRING_PROTOCOL_VERSION;
  fout.write(kPrefix, strlen(kPrefix));
  fout.write((char*)&version, sizeof(uint32_t));
  fout.write((char*)key, size);

  fout.close();

  // Rename tmp file to actual file
  int renameStatus = rename((kSaveFolder + "/" + path + ".tmp").c_str(), 
    (kSaveFolder + "/" + path).c_str());

  if(renameStatus != 0) {
    Log::Error("Could not move tmp file to actual file.");
  }
}

uint32_t SavedSessionManager::LoadKey(uint8_t* key, size_t size, const std::string& path) {
  std::ifstream fin;

  fin.open(kSaveFolder + "/" + path, kReadMode);

  if(!fin.is_open()) {
    Log::Error("File is not open.");
    return -1;
  }

  fin.seekg (0, fin.end);
  size_t fileSize = static_cast<size_t>(fin.tellg());
  fin.seekg (0, fin.beg);

  if(fileSize != (strlen(kPrefix) + sizeof(uint32_t) + size)) {
    Log::Error("File size is unexpected.");
    fin.close();
    return -1;
  }

  uint32_t version = 0;
  char prefix[strlen(kPrefix)];

  fin.read(prefix, strlen(kPrefix));
  if(fin.fail()) {
    Log::Error("Could not read prefix.");
    fin.close();
    return -1;
  }

  if(strncmp(prefix, kPrefix, strlen(kPrefix)) != 0) {
    Log::Error("Prefix strings do not match.");
    fin.close();
    return -1;
  }

  fin.read((char*)&version, sizeof(version));
  if(fin.fail()) {
    Log::Error("Failed to read version.");
    fin.close();
    return -1;
  }

  if(version == SB_PAIRING_PROTOCOL_VERSION) {
    fin.read((char*)key, size);
    if(fin.fail()) {
      Log::Error("Failed to read key.");
      fin.close();
      return -1;
    }
  }

  fin.close();

  return version;
}

void SavedSessionManager::SaveSession(const uint8_t* clientPublicKey, const uint8_t* sessionKeyEncrypt, const uint8_t* sessionKeyDecrypt) {
  std::ofstream fout;

  // Make Directory if not exist
  bool makeDirSuccess = MakeDirectory();

  if(!makeDirSuccess) {
    Log::Error("Could not successfully make directory.");
  }

  fout.open(kSaveFolder + "/" + kKnownSessionsPath + ".tmp", kWriteMode);

  if(!fout.is_open()) {
    Log::Error("Could not open file.");
    return;
  }

  uint32_t version = SB_PAIRING_PROTOCOL_VERSION;
  uint32_t numClients = 1; // for v1, this is hard coded
  fout.write(kPrefix, strlen(kPrefix));
  fout.write((char*)&version, sizeof(uint32_t));

  fout.write((char*)&numClients, sizeof(uint32_t));
  fout.write((char*)clientPublicKey, crypto_kx_PUBLICKEYBYTES);
  fout.write((char*)sessionKeyEncrypt, crypto_kx_SESSIONKEYBYTES);
  fout.write((char*)sessionKeyDecrypt, crypto_kx_SESSIONKEYBYTES);
  
  fout.close();

  // Rename tmp file to actual file
  int renameStatus = rename((kSaveFolder + "/" + kKnownSessionsPath + ".tmp").c_str(), 
    (kSaveFolder + "/" + kKnownSessionsPath).c_str());

  if(renameStatus != 0) {
    Log::Error("Could not move tmp file to actual file.");
  }
}

uint32_t SavedSessionManager::LoadSession(uint8_t* clientPublicKeyOut, uint8_t* sessionKeyEncryptOut, uint8_t* sessionKeyDecryptOut) {
  std::ifstream fin;

  fin.open(kSaveFolder + "/" + kKnownSessionsPath, kReadMode);

  if(!fin.is_open()) {
    Log::Error("File not open.");
    return -1;
  }

  uint32_t version = 0;
  uint32_t numClients = 0;
  char prefix[strlen(kPrefix)];

  fin.read(prefix, strlen(kPrefix));

  if(strncmp(prefix, kPrefix, strlen(kPrefix)) != 0) {
    Log::Error("File prefix does not match.");
    fin.close();
    return -1;
  }

  fin.read((char*)&version, sizeof(version));
  fin.read((char*)&numClients, sizeof(numClients));

  if(version == SB_PAIRING_PROTOCOL_VERSION) {
   fin.read((char*)clientPublicKeyOut, crypto_kx_PUBLICKEYBYTES);
   fin.read((char*)sessionKeyEncryptOut, crypto_kx_SESSIONKEYBYTES);
   fin.read((char*)sessionKeyDecryptOut, crypto_kx_SESSIONKEYBYTES);
  }

  fin.close();
  return version;
}

bool SavedSessionManager::MakeDirectory() {
  int status = mkdir(kSaveFolder.c_str(), S_IRUSR | S_IWUSR);

  if(status != -1 || errno == EEXIST) {
    return true;
  } else {
    return false;
  }
}

} // Switchboard
} // Anki