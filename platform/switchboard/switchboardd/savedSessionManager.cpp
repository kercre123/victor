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

const std::string SavedSessionManager::kRtsKeyPath = "/dev/block/bootdevice/by-name/switchboard";

const std::ios_base::openmode SavedSessionManager::kWriteMode = std::ios_base::binary | std::ios_base::trunc;
const std::ios_base::openmode SavedSessionManager::kReadMode = std::ios_base::binary | std::ios_base::in;
const uint8_t SavedSessionManager::kMaxNumberClients = (uint8_t)255;
const uint32_t SavedSessionManager::kNativeBufferSize = 262144; // 256 * 1024 bytes -- (256kb)
const char* SavedSessionManager::kPrefix = "ANKIBITS";

RtsKeys SavedSessionManager::LoadRtsKeys() {
  RtsKeys savedData;
  savedData.keys.version = -1;
  std::ifstream fin;

  size_t fileSize = sizeof(RtsKeysData) + (kMaxNumberClients * sizeof(RtsClientData));
  char* buffer = (char*)malloc(kNativeBufferSize);

  if(NULL == buffer) {
    Log::Error("Ran out of heap space when trying to read session keys.", kNativeBufferSize);
    return savedData;
  }

  fin.open(kRtsKeyPath, kReadMode);

  if(!fin.is_open()) {
    Log::Error("File is not open.");
    free(buffer);
    return savedData;
  }

  // Read max file size into buffer
  fin.read(buffer, fileSize);

  if(fin.fail()) {
    Log::Error("Failed to read.");
    free(buffer);
    fin.close();
    savedData.keys.version = -1;
    return savedData;
  }

  ////////////////////////////////////////////////////////////////////////
  memcpy((char*)&savedData.keys, buffer, sizeof(savedData.keys));
  
  for(int i = 0; i < savedData.keys.numKnownClients; i++) {
    RtsClientData clientData;
    char* clientSrc = buffer + sizeof(savedData.keys) + (i * sizeof(clientData));

    memcpy((char*)&(clientData), clientSrc, sizeof(clientData));
    savedData.clients.push_back(clientData);
  }

  ////////////////////////////////////////////////////////////////////////

  free(buffer);
  fin.close();

  if(strncmp((char*)&savedData.keys.magic, kPrefix, strlen(kPrefix)) != 0) {
    Log::Error("RTS keys file prefix is incorrect.");
    savedData.keys.version = -1;
    return savedData;
  }

  if(savedData.keys.version != SB_PAIRING_PROTOCOL_VERSION) {
    Log::Error("Old version of RTS keys.");
    savedData.keys.version = -1;
    return savedData;
  }

  return savedData;
}

void SavedSessionManager::SaveRtsKeys(RtsKeys& saveData) {  
  // Write file with Rts data
  std::ofstream fout;

  fout.open(kRtsKeyPath, kWriteMode);

  if(!fout.is_open()) {
    Log::Error("Could not open file.");
    return;
  }

  // Update saveData values
  memcpy((char*)&saveData.keys.magic, kPrefix, strlen(kPrefix)); 
  saveData.keys.version = SB_PAIRING_PROTOCOL_VERSION;
  saveData.keys.numKnownClients = saveData.clients.size();

  fout.write((char*)&(saveData.keys), sizeof(saveData.keys));

  for(int i = 0; i < saveData.clients.size(); i++) {
    // Write each client session
    fout.write((char*)&(saveData.clients[i]), sizeof(saveData.clients[i]));
  }

  // Zero-out the rest of our buffer size
  uint32_t numBytesToZero = kNativeBufferSize - (uint32_t)fout.tellp();
  for(int i  = 0; i < numBytesToZero; i++) {
    char zero = 0;
    fout.write((char*)&zero, sizeof(zero));
  }

  fout.close();
}

} // Switchboard
} // Anki
