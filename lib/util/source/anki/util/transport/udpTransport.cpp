/**
 * File: udpTransport
 *
 * Author: Mark Wesley
 * Created: 1/28/15
 *
 * Description: Describes an interface to UDP hardware to send and receive data
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "util/transport/udpTransport.h"
#include "util/console/consoleInterface.h"
#include "util/crc/crc.h"
#include "util/debug/messageDebugging.h"
#include "util/logging/logging.h"
#include "util/transport/iIpRetriever.h"
#include "util/transport/iNetTransportDataReceiver.h"
#include "util/transport/iUDPSocket.h"
#include "util/transport/netEmulatorUDPSocket.h"
#include "util/transport/srcBufferSet.h"
#include "util/transport/transportAddress.h"
#include <assert.h>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> // NI_DGRAM etc.
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>

#if !defined(ANDROID)
#include <ifaddrs.h>
#endif

namespace Anki {
namespace Util {
  
  
CONSOLE_VAR(bool, gUDPNetEmulatorRuntimeToggling, "Network.Emulator",  false);
CONSOLE_VAR(bool, gUDPNetEmulatorEnabled, "Network.Emulator",  false);


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-register"


// ============================== Types ========================================

typedef uint16_t  HeaderCRCType;
  
// ============================== Constants ========================================

static const size_t   kMaxNetMessageSize = 1472; // Assuming a an MTU of 1500, minus (20+8) for the underlying IP and UDP headers, this should be optimal for most networks/users

static const uint8_t  kDefaultHeaderPrefix[4] = {'A', 'N', 'K', 02}; // Prefix is equivalent to 'A''N''K'02 in Little Endian - i.e. "ANKI Drive 2", i.e. Overdrive
static const int      kDefaultPort = 47817;
  
// ============================== PosixUDPSocket ========================================

  
// Lightweight wrapper around Posix/BSD style sockets
class PosixUDPSocket : public IUDPSocket
{
public:
  PosixUDPSocket(IIpRetriever* retriever) { }
  
  virtual int OpenSocket(int protocolFamily, int socketType, int protocol) override
  {
    return socket(protocolFamily, socketType, protocol);
  }

  virtual int BindSocket(int socketId, const sockaddr* socketAddress, socklen_t socketAddressLength) override
  {
    return bind(socketId, socketAddress, socketAddressLength);
  }
  
  virtual int GetSocketOpt(int socketId, int level, int optname, void* optVal, socklen_t* optlen) override
  {
    return getsockopt(socketId, level, optname, optVal, optlen);
  }
  
  virtual int SetSocketOpt(int socketId, int level, int optname, const void* optVal, socklen_t optlen) override
  {
    return setsockopt(socketId, level, optname, optVal, optlen);
  }
  
  virtual int CloseSocket(int socketId) override
  {
    return close(socketId);
  }
  
  virtual ssize_t SendTo(int socketId, const void* messageData, size_t messageDataSize, int flags, const sockaddr* destSockAddress, socklen_t destSockAddressLength) override
  {
    return sendto(socketId, messageData, messageDataSize, flags, destSockAddress, destSockAddressLength);
  }

  virtual ssize_t ReceiveMessage(int socketId, msghdr* messageHeader, int flags) override
  {
    return recvmsg(socketId, messageHeader, flags);
  }

  virtual uint32_t GetLocalIpAddress() override
  {
    const uint32_t ipAddress = UDPTransport::GetLocalIpAddress();
    return ipAddress;
  }

};


// ============================== Helper Functions ========================================

  
void IpAddressToString(char* outBuffer, size_t outBufferLen, uint32_t inIpAddress)
{
  uint8_t* ipBytes = (uint8_t*)&inIpAddress;
  snprintf(outBuffer, outBufferLen, "%u.%u.%u.%u", ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]);
}


HeaderCRCType ComputeCRC(HeaderCRCType inCrc, const void* inBuffer, uint32_t inBufferSize)
{
  return calculate_crc_ccitt(inCrc, reinterpret_cast<const uint8_t*>(inBuffer), inBufferSize);
}
  

HeaderCRCType ComputeCRC(HeaderCRCType inCrc, const SrcBufferSet& srcBuffers)
{
  HeaderCRCType crc = inCrc;
  for (uint32_t i=0; i < srcBuffers.GetCount(); ++i)
  {
    const SizedSrcBuffer& sizedSrcBuffer = srcBuffers.GetBuffer(i);
    crc = ComputeCRC(crc, sizedSrcBuffer.GetBuffer(), sizedSrcBuffer.GetSize());
  }
  return crc;
}


// ============================== UDPTransport::HeaderPrefix ==============================

  
UDPTransport::HeaderPrefix::HeaderPrefix(const uint8_t* prefixBytes, uint32_t numBytes)
{
  Set(prefixBytes, numBytes);
}
  
  
void UDPTransport::HeaderPrefix::Set(const uint8_t* prefixBytes, uint32_t numBytes)
{
  if (numBytes > kMaxLength)
  {
    PRINT_NAMED_WARNING("UDPTransport.HeaderPrefix", "prefix too long - truncating from %u to %u", numBytes, kMaxLength);
    numBytes = kMaxLength;
  }
  memcpy(_bytes, prefixBytes, numBytes);
  _length = numBytes;
}
  
  
// ============================== UDP Packet Header ==============================
  

void UDPTransport::SetHeaderPrefix(const uint8_t* prefixBytes, uint32_t numBytes)
{
  sHeaderPrefix.Set(prefixBytes, numBytes);
}

  
const UDPTransport::HeaderPrefix& UDPTransport::GetHeaderPrefix()
{
  return sHeaderPrefix;
}

  
uint32_t UDPTransport::GetHeaderSize()
{
  const uint32_t prefixSize = sHeaderPrefix.GetLength();
  const uint32_t crcSize    = sDoesHeaderHaveCRC ? sizeof(HeaderCRCType) : 0;
  return prefixSize + crcSize;
}

  
size_t UDPTransport::GetMaxNetMessageSize()
{
  return sMaxNetMessageSize;
}

void UDPTransport::SetMaxNetMessageSize(const size_t newMTU)
{
  assert(newMTU <= kMaxNetMessageSize);
  sMaxNetMessageSize = newMTU;
}

void UDPTransport::SetDoesHeaderHaveCRC(bool hasCRC)
{
  sDoesHeaderHaveCRC = hasCRC;
}


bool UDPTransport::DoesHeaderHaveCRC()
{
  return sDoesHeaderHaveCRC;
}
  
  
UDPTransport::HeaderPrefix  UDPTransport::sHeaderPrefix(kDefaultHeaderPrefix, sizeof(kDefaultHeaderPrefix));
bool                        UDPTransport::sDoesHeaderHaveCRC = true;
size_t UDPTransport::sMaxNetMessageSize = kMaxNetMessageSize;



#if ANKI_NET_MESSAGE_LOGGING_ENABLED
const char* GetAnkiPacketHeaderDescriptor()
{
  static char     sDescriptor[128] = {0};
  static bool     sDescriptorHasCRC = false;
  static uint32_t sDescriptorPrefixLen = 0xffffffff;
  
  // build descriptor on first call
  if ((sDescriptorHasCRC != UDPTransport::DoesHeaderHaveCRC()) ||
      (sDescriptorPrefixLen != UDPTransport::GetHeaderPrefix().GetLength()))
  {
    size_t descLen = 0;
    sprintf(&sDescriptor[descLen], "|%u ch Prefix...........................", UDPTransport::GetHeaderPrefix().GetLength() );
    
    descLen = (3 * UDPTransport::GetHeaderPrefix().GetLength());
    sDescriptor[descLen++] = '|';

    if (UDPTransport::DoesHeaderHaveCRC())
    {
      const char* appendString = "-CRC-|";
      sprintf(&sDescriptor[descLen], "%s", appendString);
      descLen += strlen(appendString);
    }

    {
      const char* appendString = "Message...";
      sprintf(&sDescriptor[descLen], "%s", appendString);
      descLen += strlen(appendString);
    }
    
    assert(descLen < sizeof(sDescriptor));
    
    sDescriptorHasCRC = UDPTransport::DoesHeaderHaveCRC();
    sDescriptorPrefixLen = UDPTransport::GetHeaderPrefix().GetLength();
  }

  return sDescriptor;
}
#endif // ANKI_NET_MESSAGE_LOGGING_ENABLED
  

// ============================== PosixIpRetriever ========================================

#if !defined(ANDROID)
class PosixIpRetriever : public IIpRetriever
{
public:
  static const ifaddrs* GetIfAddrs(ifaddrs** const pointerToFree)
  {
    const ifaddrs* structToReturn = nullptr;
    bool hasEn0Address = false; // if we find multiple enX addresses, favor en0

    int errCode = getifaddrs(pointerToFree);
    if (errCode == 0)
    {
      for (ifaddrs* ifa = *pointerToFree; ifa != nullptr; ifa = ifa->ifa_next)
      {
        // is a non-null address on an enX (i.e. Wifi or Ethernet) connection
        if (ifa->ifa_addr && ifa->ifa_name && (ifa->ifa_name[0] == 'e') && (ifa->ifa_name[1] == 'n'))
        {
          if (ifa->ifa_addr->sa_family == AF_INET)
          {
            if (!hasEn0Address)
            {
              const bool isEn0Address = (ifa->ifa_name[2] == '0') && (ifa->ifa_name[3] == 0);

              hasEn0Address = isEn0Address;
              structToReturn = ifa;
            }
          }
        }
      }
    }
    return structToReturn;
  }

  static uint32_t GetSockAddress(const sockaddr_in* sockInfo)
  {
    uint32_t addressToReturn = 0;
    if (sockInfo != nullptr) {
      const void* addressStorage = &sockInfo->sin_addr;
      char addressBuffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, addressStorage, addressBuffer, INET_ADDRSTRLEN);

      addressToReturn = TransportAddress::IPAddressStringToU32(addressBuffer);
    }
    return addressToReturn;
  }

  virtual uint32_t GetIpAddress() override
  {
    uint localIpAddress = 0;

    ifaddrs* ifAddrMemory;
    const ifaddrs* ifa = GetIfAddrs(&ifAddrMemory);
    if (ifa != nullptr) {
      localIpAddress = GetSockAddress((const sockaddr_in*)ifa->ifa_addr);
    }
    freeifaddrs(ifAddrMemory);

    if (localIpAddress == 0)
    {
      // Use localhost 127.0.0.1
      localIpAddress = (127 << 24) | 1;
      PRINT_NAMED_WARNING("GetLocalIpAddressFromIfAddr", "No address found, defaulting to localhost!" );
    }

    PRINT_CH_INFO("Network", "UDPTransport.GetIpAddress", "IP address = %s", TransportAddress(localIpAddress, 0).ToString().c_str());
    
    return localIpAddress;
  }
};

static PosixIpRetriever g_PosixIpRetriever;
#endif

// ============================== Globals ========================================


#if defined(ANDROID)
static IIpRetriever* g_ipRetriever = nullptr;
#else
static IIpRetriever* g_ipRetriever = &g_PosixIpRetriever;
#endif

PosixUDPSocket g_PosixSocketImpl(g_ipRetriever);

// ============================== UDP Transport ========================================
  

UDPTransport::UDPTransport()
  : IUnreliableTransport()
  , _transportStats("UDP")
  , _udpSocketImpl(nullptr)
  , _lastSendErrorPrinted(kNetTimeStampZero)
  , _socketId(-1)
  , _port(kDefaultPort)
  , _ownsSocketImpl(false)
{
  SetSocketImpl(&g_PosixSocketImpl);
}


UDPTransport::~UDPTransport()
{
  CloseSocket();
  FreeSocketImpl();
}
  

void UDPTransport::FreeSocketImpl()
{
  if (_ownsSocketImpl)
  {
    delete _udpSocketImpl;
  }
  _udpSocketImpl  = nullptr;
  _ownsSocketImpl = false;
}

  
void UDPTransport::SetSocketImpl(IUDPSocket* udpSocketImpl, bool ownsSocketImpl)
{
  FreeSocketImpl();
  _udpSocketImpl  = udpSocketImpl;
  _ownsSocketImpl = ownsSocketImpl;
}


uint32_t UDPTransport::GetLocalIpAddress()
{
  assert(g_ipRetriever != nullptr);
  
  const uint32_t ipAddress = g_ipRetriever->GetIpAddress();
  PRINT_CH_INFO("Network", "UDPTransport.GetLocalIpAddress", "Our IP address: %s", TransportAddress(ipAddress,0).ToString().c_str());
  return ipAddress;
}

  
#if !defined(ANDROID)
uint32_t UDPTransport::GetBroadcastAddressFromIfAddr()
{
  uint32_t subnetMask = 0;
  uint32_t broadcastAddress = 0;
  uint32_t ipAddress = 0;

  ifaddrs* ifAddrMemory;
  const ifaddrs* ifa = PosixIpRetriever::GetIfAddrs(&ifAddrMemory);
  if (ifa != nullptr) {
    subnetMask = PosixIpRetriever::GetSockAddress((const sockaddr_in*)ifa->ifa_netmask);
    ipAddress = PosixIpRetriever::GetSockAddress((const sockaddr_in*)ifa->ifa_addr);
  }
  freeifaddrs(ifAddrMemory);

  if (subnetMask == 0 || ipAddress == 0) {
    // Use 255.255.255.255
    broadcastAddress = ~broadcastAddress;
    PRINT_NAMED_WARNING("GetBroadcastAddressFromIfAddr", "No address found, defaulting to 255.255.255.255!" );
  }
  else {
    broadcastAddress = (ipAddress & subnetMask) | (~subnetMask);
  }

  PRINT_CH_INFO("Network", "UDPTransport.GetBroadcastAddressFromIfAddr", "Broadcast address = %s", TransportAddress(broadcastAddress, 0).ToString().c_str());
  return broadcastAddress;
}
#endif // !defined(ANDROID)


bool UDPTransport::OpenSocket(int port)
{
  assert(_socketId == -1);
  CloseSocket();
  
  // Make sure we have the right socket impl
  UpdateSocketImplForNetEmulation();
  
  sockaddr_in socketAddress;
  memset( &socketAddress, 0, sizeof(socketAddress) );
  
  socketAddress.sin_family = AF_INET;                  // IPv4
  socketAddress.sin_addr.s_addr = htonl( INADDR_ANY ); // accept input from any address
  socketAddress.sin_port = htons( port );
  
  // Open an IP family UDP (datagram) socket

  _socketId = _udpSocketImpl->OpenSocket( PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  _port = port;
  
  if ( _socketId < 0 )
  {
    PRINT_NAMED_ERROR("UDPTransport.OpenSocketFailed", "Error: Unable to open socket - res = %d, errno = %d '%s'", _socketId, errno, strerror(errno));
    CloseSocket();
    return false;
  }
  else
  {
    PRINT_CH_INFO("Network", "UDPTransport.OpenSocket", "Opened Socket %d", _socketId);
  }
  
  int sockOptionEnable = 1;
  int setSockOptRes = -1;
  setSockOptRes = _udpSocketImpl->SetSocketOpt(_socketId, SOL_SOCKET, SO_BROADCAST, (void *)&sockOptionEnable, sizeof(sockOptionEnable));
  if (setSockOptRes < 0)
  {
    PRINT_NAMED_ERROR("UDPTransport.OpenSocket.SetBroadcastFailed.", "Unable to setsockopt SO_BROADCAST - res = %d, errno = %d '%s'", setSockOptRes, errno, strerror(errno));
    CloseSocket();
    return false;
  }

  // Bind socket (so we can send and receive on it)
  
  assert(_socketId >= 0);

  const int bindResult = _udpSocketImpl->BindSocket(_socketId, (struct sockaddr *)&socketAddress, sizeof(socketAddress));

  if ( bindResult < 0 )
  {
    if (EADDRINUSE == errno)
    {
      PRINT_NAMED_WARNING("UDPTransport.OpenSocket.BindInUse", "Warning: Unable to bind to in-use socket, continuing as this is OK in case of running multiple instances on one machine.");
    }
    else
    {
      PRINT_NAMED_ERROR("UDPTransport.OpenSocket.BindFailed", "Error: Unable to bind socket (res = %d), errno = %d '%s'", bindResult, errno, strerror(errno));
      CloseSocket();
      return false;
    }
  }
  else
  {
    PRINT_CH_INFO("Network", "UDPTransport.OpenSocket.BindSuccess", "Bind result successful - %d", bindResult );
  }
  
  PRINT_CH_INFO("Network", "UDPTransport.OpenSocket.Success", "Socket %d open on port %d. Bind %ssuccessful", _socketId, ntohs(socketAddress.sin_port), (bindResult < 0) ? "Un" : "");

  return true;
}


bool UDPTransport::CloseSocket()
{
  if (_socketId < 0)
  {
    // nothing to close
    return false;
  }

  const int closeResult = _udpSocketImpl->CloseSocket(_socketId);
  if (closeResult < 0)
  {
    PRINT_NAMED_ERROR("UDPTransport.CloseSocket.Failed", "Unable to close socket %d (res = %d), errno = %d '%s'", _socketId, closeResult, errno, strerror(errno));
  }
  else
  {
    PRINT_CH_INFO("Network", "UDPTransport.CloseSocket.Success", "Socket %d closed successfully", _socketId);
  }

  _socketId = -1;

  return (closeResult >= 0);
}
  

uint32_t BuildPacketHeader(uint8_t* outBuffer, const SrcBufferSet& srcBuffers)
{
  const UDPTransport::HeaderPrefix& headerPrefix = UDPTransport::GetHeaderPrefix();
  
  memcpy(outBuffer, headerPrefix.GetBytes(), headerPrefix.GetLength());
  uint32_t bytesWritten = headerPrefix.GetLength();
  
  if (UDPTransport::DoesHeaderHaveCRC())
  {
    HeaderCRCType crc = ComputeCRC(0, srcBuffers);
    memcpy(&outBuffer[bytesWritten], &crc, sizeof(HeaderCRCType));
    bytesWritten += sizeof(HeaderCRCType);
  }
  
  return bytesWritten;
}


uint32_t BuildPacket(uint8_t* outBuffer, uint32_t outBufferCapacity, const SrcBufferSet& srcBuffers)
{
  const uint32_t headerSize = UDPTransport::GetHeaderSize();
  
  if (outBufferCapacity < headerSize)
  {
    // Not even enough room to write the header!
    assert(0);
    return 0;
  }
  
  if (outBufferCapacity < (headerSize + srcBuffers.CalculateTotalSize()))
  {
    // Not big enough for header and entire message...
    // Should have been split at higher network level (e.g. reliablity level)
    assert(0);
    return 0;
  }
  
  uint32_t outBufferSize = BuildPacketHeader(outBuffer, srcBuffers);
  assert(outBufferSize == headerSize);

  for (uint32_t i=0; i < srcBuffers.GetCount(); ++i)
  {
    const SizedSrcBuffer& srcBuffer = srcBuffers.GetBuffer(i);
    const uint32_t srcBufferSize = srcBuffer.GetSize();
    if (srcBufferSize > 0)
    {
      memcpy(&outBuffer[outBufferSize], srcBuffer.GetBuffer(), srcBufferSize);
      outBufferSize += srcBufferSize;
    }
  }
  
  assert(outBufferSize <= outBufferCapacity);
  
  return outBufferSize;
}


ssize_t UDPTransport::SendDataToSockAddress(const sockaddr& destSockAddress, uint32_t destSockAddressLengthIn, const SrcBufferSet& srcBuffers)
{
  socklen_t destSockAddressLength = destSockAddressLengthIn;
  
  uint8_t bufferWithHeader[kMaxNetMessageSize];
  uint32_t bufferWithHeaderSize = BuildPacket(bufferWithHeader, sizeof(bufferWithHeader), srcBuffers);
 
  ANKI_NET_MESSAGE_VERBOSE(("UDP Send", GetAnkiPacketHeaderDescriptor(), bufferWithHeader, bufferWithHeaderSize, &srcBuffers));
  
  _transportStats.AddSentMessage(bufferWithHeaderSize);
  
  ssize_t sentBytes = _udpSocketImpl->SendTo( _socketId, bufferWithHeader, bufferWithHeaderSize, 0, &destSockAddress, destSockAddressLength );
  
  if (sentBytes != bufferWithHeaderSize)
  {
    if (sentBytes >= 0)
    {
      // Not an error code, but the wrong send size!?
      PRINT_NAMED_ERROR("UDPTransport.SentWrongNumBytes",
                        "sentBytes %zd != bufferSize %zd", sentBytes, bufferWithHeaderSize);
    }
  }
  
  return sentBytes;
}
  
  
void UDPTransport::SendData(const TransportAddress& destAddress, const SrcBufferSet& srcBuffers)
{
  if (destAddress.IsIPAddress())
  {
    char ipAddressString[32];
    IpAddressToString(ipAddressString, sizeof(ipAddressString), destAddress.GetIPAddress());

    sockaddr_in destSockAddress;
    memset( &destSockAddress, 0, sizeof( destSockAddress ) );
    socklen_t destSockAddressLength = static_cast<socklen_t>(sizeof( destSockAddress ));

    destSockAddress.sin_family = AF_INET; // IPv4
    hostent* host = gethostbyname( ipAddressString );

    if (host != nullptr)
    {
      // copy the address data from the host struct over to the server struct
      bcopy( host->h_addr, &(destSockAddress.sin_addr.s_addr), host->h_length);
      destSockAddress.sin_port = htons(destAddress.GetIPPort());

      ANKI_NET_PRINT_VERBOSE("SendData", "Send to '%s': %u bytes", destAddress.ToString().c_str(), srcBuffers.CalculateTotalSize());
      ssize_t sendRes = SendDataToSockAddress(destSockAddress, destSockAddressLength, srcBuffers);
      
      if (sendRes < 0)
      {
        _transportStats.AddSendError(eME_SendFailed);
        
        // when send fails it usually fails for every send (i.e. every few ms), so we throttle the warning
        // to avoid spamming
        
        const NetTimeStamp now = GetCurrentNetTimeStamp();
        const NetTimeStamp kMinSendWarningSpacingMs = 30000.0;
        if (kEnableVerboseNetworkLogging || (_lastSendErrorPrinted == kNetTimeStampZero) ||
            (now > (_lastSendErrorPrinted + kMinSendWarningSpacingMs)))
        {
          PRINT_NAMED_WARNING("UDPTransport.SendFailed", "sendto '%s' returned %zd, errno = %d '%s' (%u sends failed), now = %.1f",
                              destAddress.ToString().c_str(), sendRes, errno, strerror(errno), _transportStats.GetSentStats().GetErrorCount(eME_SendFailed), now);
          _lastSendErrorPrinted = now;
        }
      }
    }
    else
    {
      PRINT_NAMED_ERROR("UDPTransport.SendData.Failed", "Error: Failed to find host for ipAddress '%s'", ipAddressString);
    }
  }
  else
  {
    PRINT_NAMED_ERROR("UDPTransport.SendData.NonIpAddress", "Error: UDP can only send to IP addresses!");
  }
}


void UDPTransport::HandleReceivedMessage(const uint8_t* buffer, uint32_t bufferLength, const TransportAddress& sourceAddress, bool wasTruncated)
{
  ANKI_NET_MESSAGE_VERBOSE(("UDP Recv", GetAnkiPacketHeaderDescriptor(), buffer, bufferLength));
  
  const uint32_t headerSize = UDPTransport::GetHeaderSize();

  _transportStats.AddRecvMessage(bufferLength);
  
  if (bufferLength < headerSize)
  {
    // Not even big enough to hold a header - ignore (can't have been from us)
    PRINT_NAMED_WARNING("UDPTransport.BadPrefix.TooSmall", "Header '%s' is too small!",
                        ConvertMessageBufferToString(buffer, bufferLength, Util::eBTTT_Ascii).c_str());

    _transportStats.AddRecvError(eME_TooSmall);
    return;
  }
  
  const UDPTransport::HeaderPrefix& headerPrefix = UDPTransport::GetHeaderPrefix();
  
  const bool hasCorrectHeaderPrefix = (memcmp(buffer, headerPrefix.GetBytes(), headerPrefix.GetLength()) == 0);
  if (!hasCorrectHeaderPrefix)
  {
    // Message wasn't from us, random internet traffic maybe
    PRINT_NAMED_WARNING("UDPTransport.BadPrefix", "Header '%s' has wrong prefix!",
                        ConvertMessageBufferToString(buffer, headerSize, Util::eBTTT_Ascii).c_str());

    _transportStats.AddRecvError(eME_WrongHeader);
    return;
  }
  
  if (wasTruncated)
  {
    // This is bad as the message appeared to be from us (as it had the correct prefix)
    PRINT_NAMED_WARNING("UDPTransport.Recv.Truncated", "WARNING - Message buffer was too small to receive entire message and was truncated - ignoring message!");
    _transportStats.AddRecvError(eME_TooBig);
    return;
  }
  
  const uint8_t* messageBuffer = &buffer[headerSize];
  const uint32_t messageBufferLength = bufferLength - headerSize;
  
  
  if (DoesHeaderHaveCRC())
  {
    HeaderCRCType headerCrc;
    memcpy(&headerCrc, &buffer[headerPrefix.GetLength()], sizeof(HeaderCRCType));
    
    HeaderCRCType crc = ComputeCRC(0, messageBuffer, messageBufferLength);
    if (crc != headerCrc)
    {
      _transportStats.AddRecvError(eME_BadCRC);
      PRINT_NAMED_WARNING("UDPTransport.Recv.BadCRC", "WARNING - Message Has incorrect CRC (%u != %u) - ignoring message!", uint32_t(crc), uint32_t(headerCrc));
      return;
    }
  }

  if (_dataReceiver)
  {
    _dataReceiver->ReceiveData(messageBuffer, messageBufferLength, sourceAddress);
  }
}

  
bool UDPTransport::TryToReadMessage()
{
  // buffer for receiving message data
  uint8_t buffer[kMaxNetMessageSize];

  sockaddr_storage srcSockAddressStorage;
  
  const int kNumIoVecs = 1;
  iovec iov[kNumIoVecs];
  iov[0].iov_base = buffer;
  iov[0].iov_len  = sizeof(buffer);
  
  msghdr message;
  message.msg_name       = &srcSockAddressStorage;
  message.msg_namelen    = sizeof(srcSockAddressStorage);
  message.msg_iov        = iov;
  message.msg_iovlen     = kNumIoVecs;
  message.msg_control    = 0;
  message.msg_controllen = 0;
  
  ssize_t bytesReceived = _udpSocketImpl->ReceiveMessage(_socketId, &message, MSG_DONTWAIT);

  const bool wasTruncated = (message.msg_flags & MSG_TRUNC);

  if (bytesReceived > 0)
  {
    assert(bytesReceived <= kMaxNetMessageSize);
    
    const sockaddr_in* srcSockAddress = reinterpret_cast<const sockaddr_in*>(&srcSockAddressStorage);

    TransportAddress sourceTransportAddress(srcSockAddress->sin_addr.s_addr, ntohs(srcSockAddress->sin_port));
    HandleReceivedMessage(buffer, static_cast<uint32_t>(bytesReceived), sourceTransportAddress, wasTruncated);
    
    return true;
  }
  else
  {
    if (EWOULDBLOCK != errno) // EWOULDBLOCK just means there's nothing in there
    {
      PRINT_NAMED_ERROR("UDPTransport.ReadFailed",
                        "Error: recvmsg(_socketId = %d _port = %d) returned %zd, errno = %d '%s'",
                        _socketId, _port, bytesReceived, errno, strerror(errno));
    }
    
    if (ENOTCONN == errno)
    {
      const bool closeSocketSuccess = CloseSocket();
      if (!closeSocketSuccess)
      {
        PRINT_NAMED_ERROR("UDPTransport.ReadFailed.CloseSocketFailed",
                          "Error: Closing unconnected socket failed. errno = %d '%s'",
                          errno, strerror(errno));
      }
      
      if (closeSocketSuccess && !OpenSocket(_port))
      {
        PRINT_NAMED_ERROR("UDPTransport.ReadFailed.ReopenSocketFailed",
                          "Error: Reopening closed socket failed. errno = %d '%s'",
                          errno, strerror(errno));
      }
    }
    
    return false;
  }
}
  
void UDPTransport::UpdateSocketImplForNetEmulation()
{
#if REMOTE_CONSOLE_ENABLED
  if (!gUDPNetEmulatorRuntimeToggling)
  {
    return;
  }
    
  const bool wasNetEmulatorEnabled = _udpSocketImpl->IsEmulator();
  const bool isNetEmulatorEnabled  = gUDPNetEmulatorEnabled;
  if (isNetEmulatorEnabled != wasNetEmulatorEnabled)
  {
    // need to close any open sockets, switch socket impl and then reopen them
    
    bool hadOpenSocket = (_socketId >= 0);
    
    if (hadOpenSocket)
    {
      PRINT_NAMED_INFO("UDPTransport.UpdateSocketImplForNetEmulation", "Switching from %s to %s socket impl, closing and reopening socket %d",
                       wasNetEmulatorEnabled ? "NetEmu" : "Posix",  isNetEmulatorEnabled ? "NetEmu" : "Posix", _socketId);
      CloseSocket();
    }
    
    // Switch socket impl
    
    if (isNetEmulatorEnabled)
    {
      NetEmulatorUDPSocket* netEmulatorSocket = new NetEmulatorUDPSocket();
      netEmulatorSocket->SetSocketImpl(&g_PosixSocketImpl);
      SetSocketImpl(netEmulatorSocket, true);
    }
    else
    {
      SetSocketImpl(&g_PosixSocketImpl);
    }
    
    // Reopen any closed sockets
    
    if (hadOpenSocket)
    {
      OpenSocket(_port);
    }
  }
#endif // REMOTE_CONSOLE_ENABLED
}
  

void UDPTransport::Update()
{
  if (_socketId < 0)
  {
    return;
  }
  
  UpdateSocketImplForNetEmulation();
  
  // .. keep reading messages until the queue is empty
  while (TryToReadMessage())
  {
  }
}
  
  
void UDPTransport::Print() const
{
#if ANKI_NET_MESSAGE_LOGGING_ENABLED
  _transportStats.Print();
#endif // ANKI_NET_MESSAGE_LOGGING_ENABLED
}
  
  
void UDPTransport::StartHost()
{
  if (_socketId == -1)
  {
    OpenSocket(_port);
  }
}
  
  
void UDPTransport::StopHost()
{
  if (_socketId >= 0)
  {
    CloseSocket();
  }
}


void UDPTransport::StartClient()
{
  if (_socketId == -1)
  {
    OpenSocket(_port);
  }
}


void UDPTransport::StopClient()
{
  if (_socketId >= 0)
  {
    CloseSocket();
  }
}


void UDPTransport::FillAdvertisementBytes(AdvertisementBytes& bytes)
{
  // IpAddress
  {
    const uint32_t ipAddress = _udpSocketImpl->GetLocalIpAddress();
    if (0 == ipAddress)
    {
      PRINT_NAMED_WARNING("GetLocalIpAddress.IsInvalid", "localIpAddress == 0");
    }
    
    const uint8_t* const ipAddressAsBytes = reinterpret_cast<const uint8_t*>(&ipAddress);
    for (auto i = 0; i < sizeof(ipAddress); i++)
    {
      bytes.push_back(ipAddressAsBytes[i]);
    }
  }
  // Port
  {
    const uint16_t port = htons(_port);
    const uint8_t* const portAsBytes = reinterpret_cast<const uint8_t*>(&port);
    for (auto i = 0; i < sizeof(port); i++)
    {
      bytes.push_back(portAsBytes[i]);
    }
  }
}


unsigned int UDPTransport::FillAddressFromAdvertisement(TransportAddress& address, const uint8_t* buffer, unsigned int size)
{
  const uint32_t addressSize = sizeof(uint32_t) + sizeof(uint16_t);
  if (size >= addressSize)
  {
    uint32_t ipAddress = *reinterpret_cast<const uint32_t*>( buffer );
    uint16_t port      = ntohs(*reinterpret_cast<const uint16_t*>(&buffer[sizeof(ipAddress)]));
    address.SetIPAddress(ipAddress, port);
    return addressSize;
  }
  
  assert(0);
  return 0;
}
  
uint32_t UDPTransport::MaxTotalBytesPerMessage() const
{
  return static_cast<uint32_t>(sMaxNetMessageSize) - GetHeaderSize();
}

void UDPTransport::SetIpRetriever(IIpRetriever *ipRetriever)
{
  g_ipRetriever = ipRetriever;
}

#pragma GCC diagnostic pop

} // end namespace Util
} // end namespace Anki
