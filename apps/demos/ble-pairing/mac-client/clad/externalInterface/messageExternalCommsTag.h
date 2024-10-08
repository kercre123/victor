// Autogenerated C++ message buffer code.
// Source: clad/externalInterface/messageExternalComms.clad
// Full command line: victor-clad/tools/message-buffers/emitters/CPP_emitter.py --output-union-helper-constructors -C sdk -I victor-clad/clad/sdk -o generated/clad clad/externalInterface/messageExternalComms.clad

#ifndef __CLAD_EXTERNAL_INTERFACE_MESSAGE_EXTERNAL_COMMS_TAG_H__
#define __CLAD_EXTERNAL_INTERFACE_MESSAGE_EXTERNAL_COMMS_TAG_H__

#include <functional>

namespace Anki {

namespace Vector {

namespace ExternalComms {

enum class RtsConnection_2Tag : uint8_t {
  Error                      = 0x0,  // 0
  RtsConnRequest             = 0x1,  // 1
  RtsConnResponse            = 0x2,  // 2
  RtsNonceMessage            = 0x3,  // 3
  RtsChallengeMessage        = 0x4,  // 4
  RtsChallengeSuccessMessage = 0x5,  // 5
  RtsWifiConnectRequest      = 0x6,  // 6
  RtsWifiConnectResponse     = 0x7,  // 7
  RtsWifiIpRequest           = 0x8,  // 8
  RtsWifiIpResponse          = 0x9,  // 9
  RtsStatusRequest           = 0xa,  // 10
  RtsStatusResponse_2        = 0xb,  // 11
  RtsWifiScanRequest         = 0xc,  // 12
  RtsWifiScanResponse_2      = 0xd,  // 13
  RtsOtaUpdateRequest        = 0xe,  // 14
  RtsOtaUpdateResponse       = 0xf,  // 15
  RtsCancelPairing           = 0x10, // 16
  RtsForceDisconnect         = 0x11, // 17
  RtsAck                     = 0x12, // 18
  RtsWifiAccessPointRequest  = 0x13, // 19
  RtsWifiAccessPointResponse = 0x14, // 20
  RtsSshRequest              = 0x15, // 21
  RtsSshResponse             = 0x16, // 22
  RtsOtaCancelRequest        = 0x17, // 23
  RtsLogRequest              = 0x18, // 24
  RtsLogResponse             = 0x19, // 25
  RtsFileDownload            = 0x1a, // 26
  INVALID                    = 255
};

const char* RtsConnection_2TagToString(const RtsConnection_2Tag tag);

enum class RtsConnection_3Tag : uint8_t {
  Error                      = 0x0,  // 0
  RtsConnRequest             = 0x1,  // 1
  RtsConnResponse            = 0x2,  // 2
  RtsNonceMessage            = 0x3,  // 3
  RtsChallengeMessage        = 0x4,  // 4
  RtsChallengeSuccessMessage = 0x5,  // 5
  RtsWifiConnectRequest      = 0x6,  // 6
  RtsWifiConnectResponse_3   = 0x7,  // 7
  RtsWifiIpRequest           = 0x8,  // 8
  RtsWifiIpResponse          = 0x9,  // 9
  RtsStatusRequest           = 0xa,  // 10
  RtsStatusResponse_3        = 0xb,  // 11
  RtsWifiScanRequest         = 0xc,  // 12
  RtsWifiScanResponse_3      = 0xd,  // 13
  RtsOtaUpdateRequest        = 0xe,  // 14
  RtsOtaUpdateResponse       = 0xf,  // 15
  RtsCancelPairing           = 0x10, // 16
  RtsForceDisconnect         = 0x11, // 17
  RtsAck                     = 0x12, // 18
  RtsWifiAccessPointRequest  = 0x13, // 19
  RtsWifiAccessPointResponse = 0x14, // 20
  RtsSshRequest              = 0x15, // 21
  RtsSshResponse             = 0x16, // 22
  RtsOtaCancelRequest        = 0x17, // 23
  RtsLogRequest              = 0x18, // 24
  RtsLogResponse             = 0x19, // 25
  RtsFileDownload            = 0x1a, // 26
  RtsWifiForgetRequest       = 0x1b, // 27
  RtsWifiForgetResponse      = 0x1c, // 28
  RtsCloudSessionRequest     = 0x1d, // 29
  RtsCloudSessionResponse    = 0x1e, // 30
  INVALID                    = 255
};

const char* RtsConnection_3TagToString(const RtsConnection_3Tag tag);

enum class RtsConnection_4Tag : uint8_t {
  Error                      = 0x0,  // 0
  RtsConnRequest             = 0x1,  // 1
  RtsConnResponse            = 0x2,  // 2
  RtsNonceMessage            = 0x3,  // 3
  RtsChallengeMessage        = 0x4,  // 4
  RtsChallengeSuccessMessage = 0x5,  // 5
  RtsWifiConnectRequest      = 0x6,  // 6
  RtsWifiConnectResponse_3   = 0x7,  // 7
  RtsWifiIpRequest           = 0x8,  // 8
  RtsWifiIpResponse          = 0x9,  // 9
  RtsStatusRequest           = 0xa,  // 10
  RtsStatusResponse_4        = 0xb,  // 11
  RtsWifiScanRequest         = 0xc,  // 12
  RtsWifiScanResponse_3      = 0xd,  // 13
  RtsOtaUpdateRequest        = 0xe,  // 14
  RtsOtaUpdateResponse       = 0xf,  // 15
  RtsCancelPairing           = 0x10, // 16
  RtsForceDisconnect         = 0x11, // 17
  RtsAck                     = 0x12, // 18
  RtsWifiAccessPointRequest  = 0x13, // 19
  RtsWifiAccessPointResponse = 0x14, // 20
  RtsSshRequest              = 0x15, // 21
  RtsSshResponse             = 0x16, // 22
  RtsOtaCancelRequest        = 0x17, // 23
  RtsLogRequest              = 0x18, // 24
  RtsLogResponse             = 0x19, // 25
  RtsFileDownload            = 0x1a, // 26
  RtsWifiForgetRequest       = 0x1b, // 27
  RtsWifiForgetResponse      = 0x1c, // 28
  RtsCloudSessionRequest     = 0x1d, // 29
  RtsCloudSessionResponse    = 0x1e, // 30
  RtsAppConnectionIdRequest  = 0x1f, // 31
  RtsAppConnectionIdResponse = 0x20, // 32
  RtsResponse                = 0x21, // 33
  INVALID                    = 255
};

const char* RtsConnection_4TagToString(const RtsConnection_4Tag tag);

enum class RtsConnection_5Tag : uint8_t {
  Error                      = 0x0,  // 0
  RtsConnRequest             = 0x1,  // 1
  RtsConnResponse            = 0x2,  // 2
  RtsNonceMessage            = 0x3,  // 3
  RtsChallengeMessage        = 0x4,  // 4
  RtsChallengeSuccessMessage = 0x5,  // 5
  RtsWifiConnectRequest      = 0x6,  // 6
  RtsWifiConnectResponse_3   = 0x7,  // 7
  RtsWifiIpRequest           = 0x8,  // 8
  RtsWifiIpResponse          = 0x9,  // 9
  RtsStatusRequest           = 0xa,  // 10
  RtsStatusResponse_5        = 0xb,  // 11
  RtsWifiScanRequest         = 0xc,  // 12
  RtsWifiScanResponse_3      = 0xd,  // 13
  RtsOtaUpdateRequest        = 0xe,  // 14
  RtsOtaUpdateResponse       = 0xf,  // 15
  RtsCancelPairing           = 0x10, // 16
  RtsForceDisconnect         = 0x11, // 17
  RtsAck                     = 0x12, // 18
  RtsWifiAccessPointRequest  = 0x13, // 19
  RtsWifiAccessPointResponse = 0x14, // 20
  RtsSshRequest              = 0x15, // 21
  RtsSshResponse             = 0x16, // 22
  RtsOtaCancelRequest        = 0x17, // 23
  RtsLogRequest              = 0x18, // 24
  RtsLogResponse             = 0x19, // 25
  RtsFileDownload            = 0x1a, // 26
  RtsWifiForgetRequest       = 0x1b, // 27
  RtsWifiForgetResponse      = 0x1c, // 28
  RtsCloudSessionRequest_2   = 0x1d, // 29
  RtsCloudSessionResponse    = 0x1e, // 30
  RtsAppConnectionIdRequest  = 0x1f, // 31
  RtsAppConnectionIdResponse = 0x20, // 32
  RtsResponse                = 0x21, // 33
  RtsSdkProxyRequest         = 0x22, // 34
  RtsSdkProxyResponse        = 0x23, // 35
  INVALID                    = 255
};

const char* RtsConnection_5TagToString(const RtsConnection_5Tag tag);

enum class RtsConnection_1Tag : uint8_t {
  Error                      = 0x0,  // 0
  RtsConnRequest             = 0x1,  // 1
  RtsConnResponse            = 0x2,  // 2
  RtsNonceMessage            = 0x3,  // 3
  RtsChallengeMessage        = 0x4,  // 4
  RtsChallengeSuccessMessage = 0x5,  // 5
  RtsWifiConnectRequest      = 0x6,  // 6
  RtsWifiConnectResponse     = 0x7,  // 7
  RtsWifiIpRequest           = 0x8,  // 8
  RtsWifiIpResponse          = 0x9,  // 9
  RtsStatusRequest           = 0xa,  // 10
  RtsStatusResponse          = 0xb,  // 11
  RtsWifiScanRequest         = 0xc,  // 12
  RtsWifiScanResponse        = 0xd,  // 13
  RtsOtaUpdateRequest        = 0xe,  // 14
  RtsOtaUpdateResponse       = 0xf,  // 15
  RtsCancelPairing           = 0x10, // 16
  RtsForceDisconnect         = 0x11, // 17
  RtsAck                     = 0x12, // 18
  RtsWifiAccessPointRequest  = 0x13, // 19
  RtsWifiAccessPointResponse = 0x14, // 20
  RtsSshRequest              = 0x15, // 21
  RtsSshResponse             = 0x16, // 22
  INVALID                    = 255
};

const char* RtsConnection_1TagToString(const RtsConnection_1Tag tag);

enum class RtsConnectionTag : uint8_t {
  Error           = 0x0, // 0
  RtsConnection_2 = 0x2, // 2
  RtsConnection_3 = 0x3, // 3
  RtsConnection_4 = 0x4, // 4
  RtsConnection_5 = 0x5, // 5
  INVALID         = 255
};

const char* RtsConnectionTagToString(const RtsConnectionTag tag);

enum class ExternalCommsTag : uint8_t {
  Error           = 0x0, // 0
  RtsConnection_1 = 0x1, // 1
  RtsConnection   = 0x4, // 4
  INVALID         = 255
};

const char* ExternalCommsTagToString(const ExternalCommsTag tag);

} // namespace ExternalComms

} // namespace Vector

} // namespace Anki

template<>
struct std::hash<Anki::Vector::ExternalComms::RtsConnection_2Tag>
{
  size_t operator()(Anki::Vector::ExternalComms::RtsConnection_2Tag t) const
  {
    return static_cast<std::underlying_type<Anki::Vector::ExternalComms::RtsConnection_2Tag>::type>(t);
  }
};

template<>
struct std::hash<Anki::Vector::ExternalComms::RtsConnection_3Tag>
{
  size_t operator()(Anki::Vector::ExternalComms::RtsConnection_3Tag t) const
  {
    return static_cast<std::underlying_type<Anki::Vector::ExternalComms::RtsConnection_3Tag>::type>(t);
  }
};

template<>
struct std::hash<Anki::Vector::ExternalComms::RtsConnection_4Tag>
{
  size_t operator()(Anki::Vector::ExternalComms::RtsConnection_4Tag t) const
  {
    return static_cast<std::underlying_type<Anki::Vector::ExternalComms::RtsConnection_4Tag>::type>(t);
  }
};

template<>
struct std::hash<Anki::Vector::ExternalComms::RtsConnection_5Tag>
{
  size_t operator()(Anki::Vector::ExternalComms::RtsConnection_5Tag t) const
  {
    return static_cast<std::underlying_type<Anki::Vector::ExternalComms::RtsConnection_5Tag>::type>(t);
  }
};

template<>
struct std::hash<Anki::Vector::ExternalComms::RtsConnection_1Tag>
{
  size_t operator()(Anki::Vector::ExternalComms::RtsConnection_1Tag t) const
  {
    return static_cast<std::underlying_type<Anki::Vector::ExternalComms::RtsConnection_1Tag>::type>(t);
  }
};

template<>
struct std::hash<Anki::Vector::ExternalComms::RtsConnectionTag>
{
  size_t operator()(Anki::Vector::ExternalComms::RtsConnectionTag t) const
  {
    return static_cast<std::underlying_type<Anki::Vector::ExternalComms::RtsConnectionTag>::type>(t);
  }
};

template<>
struct std::hash<Anki::Vector::ExternalComms::ExternalCommsTag>
{
  size_t operator()(Anki::Vector::ExternalComms::ExternalCommsTag t) const
  {
    return static_cast<std::underlying_type<Anki::Vector::ExternalComms::ExternalCommsTag>::type>(t);
  }
};

#endif // __CLAD_EXTERNAL_INTERFACE_MESSAGE_EXTERNAL_COMMS_TAG_H__
