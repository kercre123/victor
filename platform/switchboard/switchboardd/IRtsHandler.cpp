/**
 * File: IRtsHandler.cpp
 *
 * Author: paluri
 * Created: 7/9/2018
 *
 * Description: Interface for different version of RTS protocol
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "switchboardd/IRtsHandler.h"

namespace Anki {
namespace Switchboard {

bool IRtsHandler::LoadKeys() {
  // Try to load keys
  _rtsKeys = SavedSessionManager::LoadRtsKeys();

  bool validKeys = _keyExchange->ValidateKeys((uint8_t*)&(_rtsKeys.keys.id.publicKey), (uint8_t*)&(_rtsKeys.keys.id.privateKey));

  if(!validKeys) {
    Log::Write("Keys loaded from file are corrupt.");
  } else {
    Log::Write("Stored keys are good to go.");
  }

  if(validKeys && (_rtsKeys.keys.version == SB_PAIRING_PROTOCOL_VERSION)) {
    _keyExchange->SetKeys((uint8_t*)&(_rtsKeys.keys.id.publicKey), (uint8_t*)&(_rtsKeys.keys.id.privateKey));

    Log::Write("Loading key pair from file.");
    return true;
  } else {
    // If loading keys but no keys have previously 
    // been saved, generate new ones and save them
    uint8_t* publicKey = (uint8_t*)_keyExchange->GenerateKeys();

    // Save keys to file
    memcpy(&_rtsKeys.keys.id.publicKey, publicKey, sizeof(_rtsKeys.keys.id.publicKey));
    memcpy(&_rtsKeys.keys.id.privateKey, _keyExchange->GetPrivateKey(), sizeof(_rtsKeys.keys.id.privateKey));

    SaveKeys();
    Log::Write("Generating new key pair.");
    return false;
  }
}

} // Switchboard
} // Anki