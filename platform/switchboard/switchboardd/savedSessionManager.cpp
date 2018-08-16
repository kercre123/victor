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
#include "cutils/properties.h"
#include "switchboardd/pairingMessages.h"

namespace Anki {
namespace Switchboard {

const std::string SavedSessionManager::kRtsKeyPath = "/dev/block/bootdevice/by-name/switchboard";
const std::string SavedSessionManager::kRtsKeyDataPath = "/data/data/com.anki.victor/persistent/switchboard";
const std::string SavedSessionManager::kRtsKeyDataFile = "/data/data/com.anki.victor/persistent/switchboard/sessions";

const std::ios_base::openmode SavedSessionManager::kWriteMode = std::ios_base::binary | std::ios_base::trunc;
const std::ios_base::openmode SavedSessionManager::kReadMode = std::ios_base::binary | std::ios_base::in;
const uint8_t SavedSessionManager::kMaxNumberClients = (uint8_t)255;
const uint32_t SavedSessionManager::kNativeBufferSize = 262144; // 256 * 1024 bytes -- (256kb)
const uint8_t SavedSessionManager::kMagicVersionNumber = 2; // MAGIC number that can't change
const char* SavedSessionManager::kPrefix = "ANKIBITS";

void SavedSessionManager::MigrateKeys() {
  // if switchboard /data file doesn't exist, then copy it over
  if(!HasMigrated()) {
    Log::Write("Trying to migrate keys from dev block to data.");
    // load RtsKeys from factory
    RtsKeys keys = LoadRtsKeysFactory();

    // save RtsKeys 
    SaveRtsKeys(keys);
  }
}

bool SavedSessionManager::HasMigrated() {
  std::ifstream fin(kRtsKeyDataFile);
  return fin.good();
}

std::string SavedSessionManager::GetRobotName() {
  char vicName[PROPERTY_VALUE_MAX] = {0};
  (void)property_get("anki.robot.name", vicName, "");

  return std::string(vicName);
}

RtsKeys SavedSessionManager::LoadRtsKeysFactory() {
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

  return savedData;
}

RtsKeys SavedSessionManager::LoadRtsKeys() {
  RtsKeys savedData;
  savedData.keys.version = -1;
  std::ifstream fin;

  size_t fileSize = sizeof(RtsKeysData) + (kMaxNumberClients * sizeof(RtsClientData));
  char* buffer = (char*)malloc(fileSize);

  if(NULL == buffer) {
    Log::Error("Ran out of heap space [%d] when trying to read session keys.", fileSize);
    return savedData;
  }

  fin.open(kRtsKeyDataFile, kReadMode);

  if(!fin.is_open()) {
    Log::Error("File is not open.");
    free(buffer);
    return savedData;
  }

  fin.seekg(0, fin.end);
  size_t realFileSize = static_cast<size_t>(fin.tellg());
  fin.seekg(0, fin.beg);

  if(realFileSize < sizeof(savedData.keys)) {
    Log::Error("File size is smaller than minimum expected.");
    free(buffer);
    fin.close();
    savedData.keys.version = -1;
    return savedData;
  }

  // Read max file size into buffer
  fin.read(buffer, sizeof(savedData.keys));

  if(fin.fail()) {
    Log::Error("Failed to read keys: %d, %d.", (int)fin.bad(), (int)fin.fail());
    free(buffer);
    fin.close();
    savedData.keys.version = -1;
    return savedData;
  }

  ////////////////////////////////////////////////////////////////////////
  memcpy((char*)&savedData.keys, buffer, sizeof(savedData.keys));

  size_t expectedFileSize = sizeof(savedData.keys) + savedData.keys.numKnownClients * sizeof(RtsClientData);
  if(realFileSize != expectedFileSize) {
    Log::Error("Actual file size [%d] doesn't match expected size [%d].", realFileSize, expectedFileSize);
    free(buffer);
    fin.close();
    savedData.keys.version = -1;
    return savedData;
  }

  fin.read(buffer + sizeof(savedData.keys), savedData.keys.numKnownClients * sizeof(RtsClientData));

  if(fin.fail()) {
    Log::Error("Failed to read clients: %d, %d.", (int)fin.bad(), (int)fin.fail());
    free(buffer);
    fin.close();
    savedData.keys.version = -1;
    return savedData;
  }
  
  for(int i = 0; i < savedData.keys.numKnownClients; i++) {
    RtsClientData clientData;
    char* clientSrc = buffer + sizeof(savedData.keys) + (i * sizeof(clientData));
    memcpy((char*)&(clientData), clientSrc, sizeof(clientData));
    savedData.clients.push_back(clientData);
  }

  ////////////////////////////////////////////////////////////////////////

  free(buffer);
  fin.close();
  
  char magic[128];
  memcpy(magic, (char*)&savedData.keys.magic, sizeof(savedData.keys.magic));

  if(strncmp((char*)&savedData.keys.magic, kPrefix, strlen(kPrefix)) != 0) {
    Log::Error("RTS keys file prefix is incorrect: [%s].", magic);
    savedData.keys.version = -1;
    return savedData;
  }

  return savedData;
}

void SavedSessionManager::SaveRtsKeys(RtsKeys& saveData) {  
  // Write file with Rts data
  std::ofstream fout;

  if(!MakeDirectory(kRtsKeyDataPath)) {
    Log::Write("Could not make directory.");
  }

  fout.open(kRtsKeyDataFile, kWriteMode);

  if(!fout.is_open()) {
    Log::Error("Could not open file.");
    return;
  }

  // Update saveData values
  memcpy((char*)&saveData.keys.magic, kPrefix, strlen(kPrefix)); 
  saveData.keys.version = kMagicVersionNumber;
  saveData.keys.numKnownClients = saveData.clients.size();

  fout.write((char*)&(saveData.keys), sizeof(saveData.keys));

  // If somehow we hit max clients, start removing from the beginning
  if(saveData.clients.size() > kMaxNumberClients) {
    saveData.clients.erase(saveData.clients.begin(), 
      saveData.clients.begin() + (saveData.clients.size() - kMaxNumberClients));
  }

  for(int i = 0; i < saveData.clients.size(); i++) {
    // Write each client session
    fout.write((char*)&(saveData.clients[i]), sizeof(saveData.clients[i]));
  }

  fout.close();
}

bool SavedSessionManager::MakeDirectory(std::string directory) {
  int status = mkdir(directory.c_str(), S_IRUSR | S_IWUSR);

  if(status != -1 || errno == EEXIST) {
    return true;
  } else {
    return false;
  }
}

} // Switchboard
} // Anki
