/**
 * File: pairingMessages.h
 *
 * Author: paluri
 * Created: 1/24/2018
 *
 * Description: Actual pairing messages for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef SecurePairingMessages_h
#define SecurePairingMessages_h

#include <stdlib.h>
#include "../sodium/sodium.h"

#define SB_PAIRING_PROTOCOL_VERSION 1
#define SB_PackedStruct typedef struct __attribute__((packed))
#define SB_IPv4_SIZE 4
#define SB_IPv6_SIZE 16

namespace Anki {
namespace Switchboard {
  enum PairingProtocolVersion : uint16_t {
    INVALID                   = 0,
    V1                        = 1,
    CURRENT                   = V1,
  };
  
  enum SetupMessage : uint8_t {
    MSG_RESERVED              = 0,
    MSG_ACK                   = 1,
    MSG_REQUEST_INITIAL_PAIR  = 2,
    MSG_REQUEST_RENEW         = 3,
    MSG_PUBLIC_KEY            = 4,
    MSG_NONCE                 = 5,
    MSG_CANCEL_SETUP          = 6,
    MSG_HANDSHAKE             = 7,
  };

  enum SecureMessage : uint8_t {
    CRYPTO_RESERVED           = 0,
    CRYPTO_CHALLENGE          = 1,
    CRYPTO_CHALLENGE_RESPONSE = 1,
    CRYPTO_SEND               = 2,
    CRYPTO_RECEIVE            = 3,
    CRYPTO_ACCEPTED           = 4,
    CRYPTO_WIFI_CREDENTIALS   = 5,
    CRYPTO_WIFI_STATUS        = 6,
    CRYPTO_WIFI_REQUEST_IP    = 7,
    CRYPTO_WIFI_RESPOND_IP    = 8,
    CRYPTO_UPDATE_URL         = 9,
  };
  
  enum WifiStatus : uint8_t {
    Success                   = 0,
    WrongPassword             = 1,
    Failure                   = 2,
  };
  
  enum WifiIpFlags : uint8_t {
    None                      = 0,
    Ipv4                      = 1 << 0,
    Ipv6                      = 1 << 1,
  };
  
  SB_PackedStruct Message {
  protected:
    uint8_t type;
    
  public:
    template <class T>
    static T* CastFromBuffer(char* buffer) {
      return (T*)buffer;
    }
    
    template <class T>
    static char* CastToBuffer(T* message, uint32_t* length) {
      *length = sizeof(T);
      return (char*)message;
    }
  } Message;
  
  SB_PackedStruct HandshakeMessage : Message {
  public:
    uint16_t version;
    
    HandshakeMessage(uint16_t vers) {
      version = vers;
      type = SetupMessage::MSG_HANDSHAKE;
    }
  } HandshakeMessage;
  
  SB_PackedStruct PublicKeyMessage : Message {
  public:
    char publicKey[crypto_kx_PUBLICKEYBYTES];
    
    PublicKeyMessage(char* pKey) {
      memcpy(publicKey, pKey, crypto_kx_PUBLICKEYBYTES);
      type = SetupMessage::MSG_PUBLIC_KEY;
    }
  } PublicKeyMessage;
  
  SB_PackedStruct AckMessage : Message {
  public:
    SetupMessage ackType;
    
    AckMessage(SetupMessage ackMessageType) {
      ackType = ackMessageType;
      type = SetupMessage::MSG_ACK;
    }
  } AckMessage;
  
  SB_PackedStruct InitialPairMessage : Message {
  public:
    char publicKey[crypto_kx_PUBLICKEYBYTES];
    
    InitialPairMessage(char* pKey) {
      memcpy(publicKey, pKey, crypto_kx_PUBLICKEYBYTES);
      type = SetupMessage::MSG_REQUEST_INITIAL_PAIR;
    }
  } InitialPairMessage;
  
  SB_PackedStruct RenewMessage : Message {
  public:
    char publicKey[crypto_kx_PUBLICKEYBYTES];
    
    RenewMessage(char* pKey) {
      memcpy(publicKey, pKey, crypto_kx_PUBLICKEYBYTES);
      type = SetupMessage::MSG_REQUEST_RENEW;
    }
  } RenewMessage;
  
  SB_PackedStruct NonceMessage : Message {
  public:
    char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    
    NonceMessage(char* n) {
      memcpy(nonce, n, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
      type = SetupMessage::MSG_NONCE;
    }
  } NonceMessage;
  
  SB_PackedStruct CancelMessage : Message {
  public:
    CancelMessage() {
      type = SetupMessage::MSG_CANCEL_SETUP;
    }
  } CancelMessage;
  

//    CRYPTO_SEND               = 2,
//    CRYPTO_RECEIVE            = 3,

//    CRYPTO_WIFI_STATUS        = 6,
//    CRYPTO_UPDATE_URL         = 9,
  
  SB_PackedStruct ChallengeMessage_crypto : Message {
  public:
    uint32_t number;
    
    ChallengeMessage_crypto(uint32_t n) {
      number = n;
      type = SecureMessage::CRYPTO_CHALLENGE;
    }
  } ChallengeMessage_crypto;
  
  SB_PackedStruct ChallengeAcceptedMessage_crypto : Message {
  public:
    uint8_t val = 0;
    
    ChallengeAcceptedMessage_crypto() {
      type = SecureMessage::CRYPTO_ACCEPTED;
    }
  } ChallengeAcceptedMessage_crypto;
  
  SB_PackedStruct WifiStatusMessage_crypto : Message {
  public:
    uint8_t status = 0;
    
    WifiStatusMessage_crypto() {
      type = SecureMessage::CRYPTO_WIFI_STATUS;
    }
  } WifiStatusMessage_crypto;
  
  SB_PackedStruct WifiRequestIpMessage_crypto : Message {
  public:
    WifiRequestIpMessage_crypto() {
      type = SecureMessage::CRYPTO_WIFI_REQUEST_IP;
    }
  } WifiRequestIpMessage_crypto;
  
  SB_PackedStruct WifiRespondIpMessage_crypto : Message {
  public:
    WifiIpFlags flags = WifiIpFlags::None;
    char ipv4[SB_IPv4_SIZE];
    char ipv6[SB_IPv6_SIZE];
    
    WifiRespondIpMessage_crypto(char* ip4, char* ip6) {
      if(ip4 != nullptr) {
        memcpy(ipv4, ip4, SB_IPv4_SIZE);
        flags = (WifiIpFlags)(flags | WifiIpFlags::Ipv4);
      }
      
      if(ip6 != nullptr) {
        memcpy(ipv6, ip6, SB_IPv6_SIZE);
        flags = (WifiIpFlags)(flags | WifiIpFlags::Ipv6);
      }
      
      type = SecureMessage::CRYPTO_WIFI_RESPOND_IP;
    }
  } WifiRespondIpMessage_crypto;
  
  SB_PackedStruct WifiCredentialsMessage_crypto : Message {
  public:
    uint8_t ssidLen;
    const char* wifiSsid;
    uint8_t pwLen;
    const char* wifiPw;
    
    WifiCredentialsMessage_crypto(std::string ssid, std::string pw) {
      ssidLen = ssid.length();
      wifiSsid = ssid.c_str();
      pwLen = pw.length();
      wifiPw = pw.c_str();
      type = SecureMessage::CRYPTO_WIFI_CREDENTIALS;
    }
    
    uint32_t GetBufferSize() {
      return sizeof(Message) + (sizeof(uint8_t) * 2) + ssidLen + pwLen;
    }
    
    void GetBufferCopy(char* buffer) {
      int pos = 0;
      buffer[pos++] = type;
      buffer[pos++] = ssidLen;
      memcpy(buffer + pos, wifiSsid, ssidLen);
      pos += ssidLen;
      memcpy(buffer + pos, wifiPw, pwLen);
    }
  } WifiCredentialsMessage_crypto;
} // Switchboard
} // Anki

#endif /* SecurePairingMessages_h */
