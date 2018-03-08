#include "savedSessionManager.h"
#include <sodium.h>
#include <sys/stat.h>
#include "switchboardd/pairingMessages.h"

const std::string Anki::Switchboard::SavedSessionManager::kSaveFolder = "/data/switchboard";
const std::string Anki::Switchboard::SavedSessionManager::kPublicKeyPath = "id_vec.pub";
const std::string Anki::Switchboard::SavedSessionManager::kKnownSessionsPath = "known_clients";
const std::ios_base::openmode Anki::Switchboard::SavedSessionManager::kWriteMode = std::ios_base::binary | std::ios_base::trunc;
const std::ios_base::openmode Anki::Switchboard::SavedSessionManager::kReadMode = std::ios_base::binary | std::ios_base::in;
const char* Anki::Switchboard::SavedSessionManager::kPrefix = "ANKIBITS";

void Anki::Switchboard::SavedSessionManager::SavePublicKey(const uint8_t* publicKey) {
  std::ofstream fout;

  // Make Directory if not exist
  int n = MakeDirectory();

  fout.open(kSaveFolder + "/" + kPublicKeyPath, kWriteMode);

  uint32_t version = SB_PAIRING_PROTOCOL_VERSION;
  fout.write(kPrefix, strlen(kPrefix));
  fout.write((char*)&version, sizeof(uint32_t));
  fout.write((char*)publicKey, crypto_kx_PUBLICKEYBYTES);

  fout.close();
}

uint32_t Anki::Switchboard::SavedSessionManager::LoadPublicKey(uint8_t* publicKey) {
  std::ifstream fin;

  fin.open(kSaveFolder + "/" + kPublicKeyPath, kReadMode);

  if(!fin.is_open()) {
    return -1;
  }

  uint32_t version = 0;
  char prefix[strlen(kPrefix)];

  fin.read(prefix, strlen(kPrefix));

  if(strcmp(prefix, kPrefix) != 0) {
    return -1;
  }

  fin.read((char*)&version, sizeof(version));

  if(version == SB_PAIRING_PROTOCOL_VERSION) {
   fin.read((char*)publicKey, crypto_kx_PUBLICKEYBYTES);
  }

  fin.close();

  return version;
}

void Anki::Switchboard::SavedSessionManager::SaveSession(const uint8_t* clientPublicKey, const uint8_t* sessionKeyEncrypt, const uint8_t* sessionKeyDecrypt) {
  std::ofstream fout;

  fout.open(kSaveFolder + "/" + kKnownSessionsPath, kWriteMode);

  uint32_t version = SB_PAIRING_PROTOCOL_VERSION;
  uint32_t numClients = 1; // for v1, this is hard coded
  fout.write(kPrefix, strlen(kPrefix));
  fout.write((char*)&version, sizeof(uint32_t));

  fout.write((char*)&numClients, sizeof(uint32_t));
  fout.write((char*)clientPublicKey, crypto_kx_PUBLICKEYBYTES);
  fout.write((char*)sessionKeyEncrypt, crypto_kx_SESSIONKEYBYTES);
  fout.write((char*)sessionKeyDecrypt, crypto_kx_SESSIONKEYBYTES);
  
  fout.close();
}

uint32_t Anki::Switchboard::SavedSessionManager::LoadSession(uint8_t* clientPublicKeyOut, uint8_t* sessionKeyEncryptOut, uint8_t* sessionKeyDecryptOut) {
  std::ifstream fin;

  fin.open(kSaveFolder + "/" + kPublicKeyPath, kReadMode);

  if(!fin.is_open()) {
    return -1;
  }

  uint32_t version = 0;
  uint32_t numClients = 0;
  char prefix[strlen(kPrefix)];

  fin.read(prefix, strlen(kPrefix));

  if(strcmp(prefix, kPrefix) != 0) {
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

int Anki::Switchboard::SavedSessionManager::MakeDirectory() {
  return mkdir(kSaveFolder.c_str(), S_IRUSR | S_IWUSR);
}