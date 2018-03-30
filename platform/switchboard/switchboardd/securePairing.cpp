/**
 * File: securePairing.cpp
 *
 * Author: paluri
 * Created: 1/16/2018
 *
 * Description: Secure Pairing controller for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "anki-wifi/wifi.h"
#include "switchboardd/savedSessionManager.h"
#include "switchboardd/securePairing.h"
#include "exec_command.h"

#include "anki-wifi/fileutils.h" // RtsSsh

#include <sstream>
#include <cutils/properties.h>

namespace Anki {
namespace Switchboard {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Constructors
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
long long SecurePairing::sTimeStarted;

SecurePairing::SecurePairing(INetworkStream* stream, 
  struct ev_loop* evloop,
  std::shared_ptr<EngineMessagingClient> engineClient,
  bool isPairing, 
  bool isOtaUpdating) :
_pin(""),
_challengeAttempts(0),
_totalPairingAttempts(0),
_numPinDigits(0),
_pingChallenge(0),
_abnormalityCount(0),
_inetTimerCount(0),
_wifiConnectTimeout_s(15),
_commsState(CommsState::Raw),
_stream(stream),
_loop(evloop),
_engineClient(engineClient),
_isPairing(isPairing),
_isOtaUpdating(isOtaUpdating)
{
  Log::Write("Instantiate with isPairing:%s", isPairing?"true":"false");
  sTimeStarted = std::time(0);
  
  // Register with stream events
  _onReceivePlainTextHandle = _stream->OnReceivedPlainTextEvent().ScopedSubscribe(
    std::bind(&SecurePairing::HandleMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _onReceiveEncryptedHandle = _stream->OnReceivedEncryptedEvent().ScopedSubscribe(
    std::bind(&SecurePairing::HandleMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _onFailedDecryptionHandle = _stream->OnFailedDecryptionEvent().ScopedSubscribe(
    std::bind(&SecurePairing::HandleDecryptionFailed, this));
  
  // Register with private events
  _pairingTimeoutSignal.SubscribeForever(std::bind(&SecurePairing::HandleTimeout, this));
  _internetTimerSignal.SubscribeForever(std::bind(&SecurePairing::HandleInternetTimerTick, this));
  
  // Initialize the key exchange object
  _keyExchange = std::make_unique<KeyExchange>(kNumPinDigits);

  // Initialize the message handler
  _cladHandler = std::make_unique<ExternalCommsCladHandler>();
  SubscribeToCladMessages();

  // Initialize the task executor
  _taskExecutor = std::make_unique<TaskExecutor>(_loop);
  
  // Initialize ev timer
  Log::Write("[timer] init");
  _handleTimeoutTimer.signal = &_pairingTimeoutSignal;
  ev_timer_init(&_handleTimeoutTimer.timer, &SecurePairing::sEvTimerHandler, kPairingTimeout_s, kPairingTimeout_s);

  _handleInternet.signal = &_internetTimerSignal;
  ev_timer_init(&_handleInternet.timer, &SecurePairing::sEvTimerHandler, kWifiConnectInterval_s, kWifiConnectInterval_s);
  
  Log::Write("SecurePairing starting up.");
}

SecurePairing::~SecurePairing() {
  _onReceivePlainTextHandle = nullptr;
  _onReceiveEncryptedHandle = nullptr;
  _onFailedDecryptionHandle = nullptr;

  ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
  ev_timer_stop(_loop, &_handleInternet.timer);
  
  Log::Write("Destroying SecurePairing object.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Initialization/Reset methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void SecurePairing::BeginPairing() {
  Log::Write("Beginning secure pairing process.");
  
  _totalPairingAttempts = 0;
  
  // Initialize object
  Init();
}

void SecurePairing::StopPairing() {
  _totalPairingAttempts = kMaxPairingAttempts;
  
  Reset(true);
}

void SecurePairing::Init() {
  // Clear field values
  _challengeAttempts = 0;
  _abnormalityCount = 0;
  _inetTimerCount = 0;
  
  // Update our state
  _state = PairingState::Initial;
  
  // Send Handshake
  Log::Write("Sending Handshake to Client.");
  
  ev_timer_again(_loop, &_handleTimeoutTimer.timer);
  
  SendHandshake();
  
  _state = PairingState::AwaitingHandshake;
}

void SecurePairing::SubscribeToCladMessages() {
  _rtsConnResponseHandle = _cladHandler->OnReceiveRtsConnResponse().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsConnResponse, this, std::placeholders::_1));
  _rtsChallengeMessageHandle = _cladHandler->OnReceiveRtsChallengeMessage().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsChallengeMessage, this, std::placeholders::_1));
  _rtsWifiConnectRequestHandle = _cladHandler->OnReceiveRtsWifiConnectRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsWifiConnectRequest, this, std::placeholders::_1));
  _rtsWifiIpRequestHandle = _cladHandler->OnReceiveRtsWifiIpRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsWifiIpRequest, this, std::placeholders::_1));
  _rtsRtsStatusRequestHandle = _cladHandler->OnReceiveRtsStatusRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsStatusRequest, this, std::placeholders::_1));
  _rtsWifiScanRequestHandle = _cladHandler->OnReceiveRtsWifiScanRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsWifiScanRequest, this, std::placeholders::_1));
  _rtsOtaUpdateRequestHandle = _cladHandler->OnReceiveRtsOtaUpdateRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsOtaUpdateRequest, this, std::placeholders::_1));
  _rtsWifiAccessPointRequestHandle = _cladHandler->OnReceiveRtsWifiAccessPointRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsWifiAccessPointRequest, this, std::placeholders::_1));
  _rtsCancelPairingHandle = _cladHandler->OnReceiveCancelPairingRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsCancelPairing, this, std::placeholders::_1));
  _rtsAckHandle = _cladHandler->OnReceiveRtsAck().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsAck, this, std::placeholders::_1));
  _rtsSshHandle = _cladHandler->OnReceiveRtsSsh().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsSsh, this, std::placeholders::_1));
}

void SecurePairing::Reset(bool forced) {
  // Tell the stream that we can no longer send over encrypted channel
  _stream->SetEncryptedChannelEstablished(false);

  // Send cancel message -- must do this before state is RAW
  SendCancelPairing();

  _commsState = CommsState::Raw;
  
  // Tell key exchange to reset
  _keyExchange->Reset();
  
  // Clear pin
  _pin = "000000";

  // Stop timers
  ev_timer_stop(_loop, &_handleInternet.timer);
  
  // Put us back in initial state
  if(forced) {
    Log::Write("Client disconnected. Stopping pairing.");
    ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
  } else if(++_totalPairingAttempts < kMaxPairingAttempts) {
    Init();
    Log::Write("SecurePairing restarting.");
  } else {
    Log::Write("SecurePairing ending due to multiple failures. Requires external restart.");
    ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Send data methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

using namespace Anki::Victor::ExternalComms;

void SecurePairing::SendHandshake() {
  if(!AssertState(CommsState::Raw)) {
    return;
  }
  // Send versioning handshake
  // ************************************************************
  // Handshake Message (first message)
  // This message is fixed. Cannot change. Ever. 
  // If you are thinking about changing the code in this message,
  // DON'T. All Victor's for all time must send this message.
  // ANY victor needs to be able to communicate with
  // ANY version of the client, at least enough to
  // know if they can speak the same language.
  // ************************************************************
  const uint8_t kHandshakeMessageLength = 5;
  uint8_t handshakeMessage[kHandshakeMessageLength];
  handshakeMessage[0] = SetupMessage::MSG_HANDSHAKE;
  *(uint32_t*)(&handshakeMessage[1]) = PairingProtocolVersion::CURRENT;
  int result = _stream->SendPlainText(handshakeMessage, sizeof(handshakeMessage));

  if(result != 0) {
    Log::Write("Unable to send message.");
  }
}

void SecurePairing::SendPublicKey() {
  if(!AssertState(CommsState::Clad)) {
    return;
  }

  // Generate public, private key
  uint8_t* publicKey;

  uint8_t publicKeyBuffer[crypto_kx_PUBLICKEYBYTES];
  uint8_t privateKeyBuffer[crypto_kx_SECRETKEYBYTES];
  uint32_t fileVersion = SavedSessionManager::LoadKey(&publicKeyBuffer[0], crypto_kx_PUBLICKEYBYTES, SavedSessionManager::kPublicKeyPath);
  uint32_t fileVPrivate = SavedSessionManager::LoadKey(&privateKeyBuffer[0], crypto_kx_SECRETKEYBYTES, SavedSessionManager::kPrivateKeyPath);

  // Check if keys from file are valid
  bool validKeys = _keyExchange->ValidateKeys(publicKeyBuffer, privateKeyBuffer);

  if(!validKeys) {
    Log::Write("Keys loaded from file are corrupt.");
  } else {
    Log::Write("Store keys are good to go.");
  }

  if(validKeys && (fileVersion == SB_PAIRING_PROTOCOL_VERSION) && (fileVPrivate == SB_PAIRING_PROTOCOL_VERSION)) {
    publicKey = publicKeyBuffer;
    _keyExchange->SetKeys(publicKey, privateKeyBuffer);

    Log::Write("Loading key pair from file.");
  } else {
    publicKey = (uint8_t*)_keyExchange->GenerateKeys();

    // Save keys to file
    SavedSessionManager::SaveKey(publicKey, crypto_kx_PUBLICKEYBYTES, SavedSessionManager::kPublicKeyPath);
    SavedSessionManager::SaveKey(_keyExchange->GetPrivateKey(), crypto_kx_SECRETKEYBYTES, SavedSessionManager::kPrivateKeyPath);
    Log::Write("Generating new key pair.");
  }

  std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKeyArray;
  memcpy(std::begin(publicKeyArray), publicKey, crypto_kx_PUBLICKEYBYTES);
  SendRtsMessage<RtsConnRequest>(publicKeyArray);

  // Save public key to file
  Log::Write("Sending public key to client.");
}

void SecurePairing::SendNonce() {
  if(!AssertState(CommsState::Clad)) {
    return;
  }

  // Send nonce
  const uint8_t NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
  
  // Generate a nonce
  uint8_t* toRobotNonce = _keyExchange->GetToRobotNonce();
  randombytes_buf(toRobotNonce, NONCE_BYTES);

  uint8_t* toDeviceNonce = _keyExchange->GetToDeviceNonce();
  randombytes_buf(toDeviceNonce, NONCE_BYTES);
  
  // Give our nonce to the network stream
  _stream->SetNonce(toRobotNonce, toDeviceNonce);
  
  std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES> toRobotNonceArray;
  memcpy(std::begin(toRobotNonceArray), toRobotNonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);

  std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES> toDeviceNonceArray;
  memcpy(std::begin(toDeviceNonceArray), toDeviceNonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);

  SendRtsMessage<RtsNonceMessage>(toRobotNonceArray, toDeviceNonceArray);
}

void SecurePairing::SendChallenge() {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  // Tell the stream that we can now send over encrypted channel
  _stream->SetEncryptedChannelEstablished(true);
  // Update state to secureClad
  _state = PairingState::AwaitingChallengeResponse;
  
  // Create random challenge value
  randombytes_buf(&_pingChallenge, sizeof(_pingChallenge));
  
  SendRtsMessage<RtsChallengeMessage>(_pingChallenge);
}

void SecurePairing::SendChallengeSuccess() {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  // Send challenge and update state
  SendRtsMessage<RtsChallengeSuccessMessage>();
}

void SecurePairing::SendWifiScanResult() {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  // TODO: will replace with CLAD message format
  std::vector<Anki::WiFiScanResult> wifiResults = Anki::ScanForWiFiAccessPoints();

  const uint8_t statusCode = wifiResults.size() > 0? 0 : 1;

  std::vector<Anki::Victor::ExternalComms::RtsWifiScanResult> wifiScanResults;

  for(int i = 0; i < wifiResults.size(); i++) {
    Anki::Victor::ExternalComms::RtsWifiScanResult result = Anki::Victor::ExternalComms::RtsWifiScanResult(wifiResults[i].auth,
      wifiResults[i].signal_level,
      wifiResults[i].ssid);

      wifiScanResults.push_back(result);
  }

  Log::Write("Sending wifi scan results.");
  SendRtsMessage<RtsWifiScanResponse>(statusCode, wifiScanResults);
}

void SecurePairing::SendWifiConnectResult() {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  // Send challenge and update state
  WiFiState wifiState = Anki::GetWiFiState();
  SendRtsMessage<RtsWifiConnectResponse>(wifiState.ssid, wifiState.connState);
}

void SecurePairing::SendWifiAccessPointResponse(bool success, std::string ssid, std::string pw) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  // Send challenge and update state
  SendRtsMessage<RtsWifiAccessPointResponse>(success, ssid, pw);
}

void SecurePairing::SendStatusResponse() {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  WiFiState state = Anki::GetWiFiState();
  uint8_t bleState = 1; // for now, if we are sending this message, we are connected
  uint8_t batteryState = 0; // for now, ignore this field until we have a way to get that info
  bool isApMode = Anki::IsAccessPointMode();

  // Send challenge and update state
  char buildNo[PROPERTY_VALUE_MAX] = {0};
  (void)property_get("ro.build.id", buildNo, "");

  std::string buildNoString(buildNo);

  SendRtsMessage<RtsStatusResponse_2>(state.ssid, state.connState, isApMode, bleState, batteryState, buildNoString);

  Log::Write("Send status response.");
}

void SecurePairing::SendCancelPairing() {
  // Send challenge and update state
  SendRtsMessage<RtsCancelPairing>();
  Log::Write("Canceling pairing.");
}

void SecurePairing::SendOtaProgress(int status, uint64_t progress, uint64_t expectedTotal) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }
  // Send Ota Progress
  SendRtsMessage<RtsOtaUpdateResponse>(status, progress, expectedTotal);
  Log::Write("Sending OTA Progress Update");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Event handling methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void SecurePairing::HandleRtsConnResponse(const Anki::Victor::ExternalComms::RtsConnection_2& msg) {
  if(!AssertState(CommsState::Clad)) {
    return;
  }

  if(_state == PairingState::AwaitingPublicKey) {
    Anki::Victor::ExternalComms::RtsConnResponse connResponse = msg.Get_RtsConnResponse();

    if(connResponse.connectionType == Anki::Victor::ExternalComms::RtsConnType::FirstTimePair) {    
      if(_isPairing && !_isOtaUpdating) {
        HandleInitialPair((uint8_t*)connResponse.publicKey.data(), crypto_kx_PUBLICKEYBYTES);
        _state = PairingState::AwaitingNonceAck;
      } else {
        Log::Write("Client tried to initial pair while not in pairing mode.");
      }
    } else {
      uint8_t clientPublicKeySaved[crypto_kx_PUBLICKEYBYTES];
      uint8_t clientEncryptKeySaved[crypto_kx_SESSIONKEYBYTES];
      uint8_t clientDecryptKeySaved[crypto_kx_SESSIONKEYBYTES];

      int sessionVersion = SavedSessionManager::LoadSession(clientPublicKeySaved, clientEncryptKeySaved, clientDecryptKeySaved);

      if(sessionVersion == SB_PAIRING_PROTOCOL_VERSION && 
        (memcmp((uint8_t*)connResponse.publicKey.data(), clientPublicKeySaved, crypto_kx_PUBLICKEYBYTES) == 0)) {
        _stream->SetCryptoKeys(
          clientEncryptKeySaved,
          clientDecryptKeySaved);

        SendNonce();
        _state = PairingState::AwaitingNonceAck;
        Log::Write("Received renew connection request.");
      } else {
        Reset();
        Log::Write("No stored session for public key.");
      }
    }
  } else {
    // ignore msg
    IncrementAbnormalityCount();
    Log::Write("Received initial pair request in wrong state.");
  }
}

void SecurePairing::HandleRtsChallengeMessage(const Victor::ExternalComms::RtsConnection_2& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::AwaitingChallengeResponse) {
    Anki::Victor::ExternalComms::RtsChallengeMessage challengeMessage = msg.Get_RtsChallengeMessage();

    HandleChallengeResponse((uint8_t*)&challengeMessage.number, sizeof(challengeMessage.number));
  } else {
    // ignore msg
    IncrementAbnormalityCount();
    Log::Write("Received challenge response in wrong state.");
  }
}

void SecurePairing::HandleRtsWifiConnectRequest(const Victor::ExternalComms::RtsConnection_2& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::ConfirmedSharedSecret) {
    Anki::Victor::ExternalComms::RtsWifiConnectRequest wifiConnectMessage = msg.Get_RtsWifiConnectRequest();

    Log::Write("Trying to connect to wifi network [%s][pw=%s][sec=%d][hid=%d].", wifiConnectMessage.wifiSsidHex.c_str(), wifiConnectMessage.password.c_str(), wifiConnectMessage.authType, wifiConnectMessage.hidden);

    _wifiConnectTimeout_s = std::max(kWifiConnectMinTimeout_s, wifiConnectMessage.timeout);

    UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::SETTING_WIFI);

    bool connected = Anki::ConnectWiFiBySsid(wifiConnectMessage.wifiSsidHex, 
      wifiConnectMessage.password,
      wifiConnectMessage.authType,
      (bool)wifiConnectMessage.hidden,
      nullptr,
      nullptr);

    WiFiState state = Anki::GetWiFiState();
    bool online = state.connState == WiFiConnState::ONLINE;

    if(online) {
      SendWifiConnectResult();
    } else {
      ev_timer_again(_loop, &_handleInternet.timer);
    }

    if(connected) {
      Log::Write("Connected to wifi.");
    } else {
      Log::Write("Could not connect to wifi.");
    }
  } else {
    Log::Write("Received wifi credentials in wrong state.");
  }
}

void SecurePairing::HandleRtsWifiIpRequest(const Victor::ExternalComms::RtsConnection_2& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::ConfirmedSharedSecret) {
    std::array<uint8_t, 4> ipV4;
    std::array<uint8_t, 16> ipV6;

    Anki::GetIpAddress(ipV4.data(), ipV6.data());
    SendRtsMessage<RtsWifiIpResponse>(1, 0, ipV4, ipV6);
  }
  
  Log::Write("Received wifi ip request.");
}

void SecurePairing::HandleRtsStatusRequest(const Victor::ExternalComms::RtsConnection_2& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::ConfirmedSharedSecret) {
    SendStatusResponse();
  } else {
    Log::Write("Received status request in the wrong state.");
  }
}

void SecurePairing::HandleRtsWifiScanRequest(const Victor::ExternalComms::RtsConnection_2& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::ConfirmedSharedSecret) {
    UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::SETTING_WIFI);
    SendWifiScanResult();
  } else {
    Log::Write("Received wifi scan request in wrong state.");
  }
}

void SecurePairing::HandleRtsOtaUpdateRequest(const Victor::ExternalComms::RtsConnection_2& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::ConfirmedSharedSecret && !_isOtaUpdating) {
    Anki::Victor::ExternalComms::RtsOtaUpdateRequest otaMessage = msg.Get_RtsOtaUpdateRequest();
    _otaUpdateRequestSignal.emit(otaMessage.url);
    _isOtaUpdating = true;
  }
  
  Log::Write("Starting OTA update.");
}

void SecurePairing::HandleRtsWifiAccessPointRequest(const Victor::ExternalComms::RtsConnection_2& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::ConfirmedSharedSecret) {
    Anki::Victor::ExternalComms::RtsWifiAccessPointRequest accessPointMessage = msg.Get_RtsWifiAccessPointRequest();
    if(accessPointMessage.enable) {
      // enable access point mode on Victor
      char vicName[PROPERTY_VALUE_MAX] = {0};
      (void)property_get("persist.anki.robot.name", vicName, "");

      std::string ssid(vicName);
      std::string password = _keyExchange->GeneratePin(kWifiApPasswordSize);

      UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::SETTING_WIFI);

      bool success = Anki::EnableAccessPointMode(ssid, password);

      SendWifiAccessPointResponse(success, ssid, password);

      Log::Write("Received request to enter wifi access point mode.");
    } else {
      // disable access point mode on Victor
      bool success = Anki::DisableAccessPointMode();

      SendWifiAccessPointResponse(success, "", "");

      Log::Write("Received request to disable access point mode.");
    }
  }
}

void SecurePairing::HandleRtsCancelPairing(const Victor::ExternalComms::RtsConnection_2& msg) {
  Log::Write("Stopping pairing due to client request.");
  StopPairing();
}

void SecurePairing::HandleRtsSsh(const Victor::ExternalComms::RtsConnection_2& msg) {
  // RtsSsh
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::ConfirmedSharedSecret) {
    Anki::Victor::ExternalComms::RtsSshRequest sshMsg = msg.Get_RtsSshRequest();
    std::string sshPath = "/home/root/.ssh";
    std::string sshFile = "authorized_keys";

    CreateDirectory(sshPath);

    std::string data = "";

    for(int i = 0; i < sshMsg.sshAuthorizedKeyBytes.size(); i++) {
      data += sshMsg.sshAuthorizedKeyBytes[i];
    }

    Log::Write("Writing public SSH key to file: %s/%s", sshPath.c_str(), sshFile.c_str());
    WriteFileAtomically(sshPath + "/" + sshFile, data, Anki::kModeUserReadWrite);
  }
}

void SecurePairing::HandleRtsAck(const Victor::ExternalComms::RtsConnection_2& msg) {
  Anki::Victor::ExternalComms::RtsAck ack = msg.Get_RtsAck();
  if(_state == PairingState::AwaitingNonceAck && 
    ack.rtsConnectionTag == (uint8_t)Anki::Victor::ExternalComms::RtsConnection_2Tag::RtsNonceMessage) {
    HandleNonceAck();
  } else {
    // ignore msg
    IncrementAbnormalityCount();
    
    std::ostringstream ss;
    ss << "Received nonce ack in wrong state '" << _state << "'.";
    Log::Write(ss.str().c_str());
  }
}

bool SecurePairing::HandleHandshake(uint16_t version) {
  // Todo: in the future, when there are more versions,
  // this method will need to handle telling this object
  // to properly switch states to adjust to proper version.
  if(version == PairingProtocolVersion::CURRENT) {
    return true;
  }
  else if(version == PairingProtocolVersion::INVALID) {
    // Client should never send us this message.
    Log::Write("Client reported incompatible version [%d]. Our version is [%d]", version, PairingProtocolVersion::CURRENT);
    return false;
  }
  
  return false;
}

void SecurePairing::HandleInitialPair(uint8_t* publicKey, uint32_t publicKeyLength) {
  // Handle initial pair request from client
  
  // Generate a random number with kNumPinDigits digits
  _pin = _keyExchange->GeneratePin();
  _updatedPinSignal.emit(_pin);
  
  // Input client's public key and calculate shared keys
  _keyExchange->SetRemotePublicKey(publicKey);
  _keyExchange->CalculateSharedKeys((unsigned char*)_pin.c_str());
  
  // Give our shared keys to the network stream
  _stream->SetCryptoKeys(
    _keyExchange->GetEncryptKey(),
    _keyExchange->GetDecryptKey());

  // Save keys to file
  SavedSessionManager::SaveSession(
    publicKey,
    _keyExchange->GetEncryptKey(),
    _keyExchange->GetDecryptKey());
  
  // Send nonce
  SendNonce();
  
  Log::Write("Received initial pair request, sending nonce.");
}

void SecurePairing::HandleTimeout() {
  if(_state != PairingState::ConfirmedSharedSecret) {
    Log::Write("Pairing timeout. Client took too long.");
    Reset();
  }
}

void SecurePairing::HandleDecryptionFailed() {
  Log::Write("Decryption failed...");
  Reset();
}

void SecurePairing::HandleNonceAck() {
  // Send challenge to user
  _commsState = CommsState::SecureClad;
  SendChallenge();
  
  Log::Write("Client acked nonce, sending challenge [%d].", _pingChallenge);
}

inline bool isChallengeSuccess(uint32_t challenge, uint32_t answer) {
  const bool isSuccess = answer == challenge + 1;
  return isSuccess;
}

void SecurePairing::HandleChallengeResponse(uint8_t* pingChallengeAnswer, uint32_t length) {
  bool success = false;
  
  if(length < sizeof(uint32_t)) {
    success = false;
  } else {
    uint32_t answer = *((uint32_t*)pingChallengeAnswer);
    success = isChallengeSuccess(_pingChallenge, answer);
  }
  
  if(success) {
    // Inform client that we are good to go and
    // update our state
    SendChallengeSuccess();
    _state = PairingState::ConfirmedSharedSecret;
    Log::Green("Challenge answer was accepted. Encrypted channel established.");
  } else {
    // Increment our abnormality and attack counter, and
    // if at or above max attempts reset.
    IncrementAbnormalityCount();
    IncrementChallengeCount();
    Log::Write("Received faulty challenge response.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Receive messages method
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void SecurePairing::HandleMessageReceived(uint8_t* bytes, uint32_t length) {
  _taskExecutor->WakeSync([this, bytes, length]() {
    if(length < kMinMessageSize) {
      Log::Write("Length is less than kMinMessageSize.");
      return;
    }

    if(_commsState == CommsState::Raw) {
      if(_state == PairingState::Initial ||
        _state == PairingState::AwaitingHandshake) {
        // ************************************************************
        // Handshake Message (first message)
        // This message is fixed. Cannot change. Ever.
        // ANY victor needs to be able to communicate with
        // ANY version of the client, at least enough to
        // know if they can speak the same language.
        // ************************************************************
        if((SetupMessage)bytes[0] == SetupMessage::MSG_HANDSHAKE) {
          bool handleHandshake = false;

          if(length < sizeof(uint32_t) + 1) {
            Log::Write("Handshake message too short.");
          } else {
            uint32_t clientVersion = *(uint32_t*)(bytes + 1);
            handleHandshake = HandleHandshake(clientVersion);
          }
          
          if(handleHandshake) {
            _commsState = CommsState::Clad;
            SendPublicKey();
            _state = PairingState::AwaitingPublicKey;
          } else {
            // If we can't handle handshake, must cancel
            // THIS SHOULD NEVER HAPPEN
            Log::Write("Unable to process handshake. Something very bad happened.");
            StopPairing();
          }
        } else {
          // ignore msg
        IncrementAbnormalityCount();
        Log::Write("Received raw message that is not handshake.");
        }
      } else{
        // ignore msg
        IncrementAbnormalityCount();
        Log::Write("Internal state machine error. Assuming raw message, but state is not initial [%d].", (int)_state);
      }
    } else {
      _cladHandler->ReceiveExternalCommsMsg(bytes, length);
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void SecurePairing::IncrementChallengeCount() {
  // Increment challenge count
  _challengeAttempts++;
  
  if(_challengeAttempts >= kMaxMatchAttempts) {
    Reset();
  }
  
  Log::Write("Client answered challenge.");
}

void SecurePairing::IncrementAbnormalityCount() {
  // Increment abnormality count
  _abnormalityCount++;
  
  if(_abnormalityCount >= kMaxAbnormalityCount) {
    Reset();
  }
  
  Log::Write("Abnormality recorded.");
}

void SecurePairing::HandleInternetTimerTick() {
  _inetTimerCount++;

  WiFiState state = Anki::GetWiFiState();
  bool online = state.connState == WiFiConnState::ONLINE;

  if(online || _inetTimerCount > _wifiConnectTimeout_s) {
    ev_timer_stop(_loop, &_handleInternet.timer);
    SendWifiConnectResult();
  }
}

void SecurePairing::UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus state) {
  if(!_isOtaUpdating) {
    _engineClient->ShowPairingStatus(state);
  } else {
    _engineClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::UPDATING_OS);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Send messages method
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<typename T, typename... Args>
int SecurePairing::SendRtsMessage(Args&&... args) {
  Anki::Victor::ExternalComms::ExternalComms msg = Anki::Victor::ExternalComms::ExternalComms(
    Anki::Victor::ExternalComms::RtsConnection(Anki::Victor::ExternalComms::RtsConnection_2(T(std::forward<Args>(args)...))));
  std::vector<uint8_t> messageData(msg.Size());
  const size_t packedSize = msg.Pack(messageData.data(), msg.Size());

  if(_commsState == CommsState::Clad) {
    return _stream->SendPlainText(messageData.data(), packedSize);
  } else if(_commsState == CommsState::SecureClad) {
    return _stream->SendEncrypted(messageData.data(), packedSize);
  } else {
    Log::Write("Tried to send clad message when state was already set back to RAW.");
  }

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Static methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void SecurePairing::sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents)
{
  std::ostringstream ss;
  ss << "[timer] " << (time(0) - sTimeStarted) << "s since beginning.";
  Log::Write(ss.str().c_str());
  
  struct ev_TimerStruct *wData = (struct ev_TimerStruct*)w;
  wData->signal->emit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// EOF
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
} // Switchboard
} // Anki