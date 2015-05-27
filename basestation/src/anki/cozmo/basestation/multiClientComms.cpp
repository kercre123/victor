/**
 * File: multiClientComms.cpp
 *
 * Author: Kevin Yoon
 * Created: 1/22/2014
 *
 * Description: Interface class that creates multiple TCP or UDP clients to connect
 *              and communicate with advertising devices.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/cozmo/basestation/multiClientComms.h"

#include "anki/util/logging/logging.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include <utility>

// simple adapter class that calls std::get<I> on the tuple result
// so it also works with pairs or anything that supports std::get<I>
template<size_t I, typename Iter>
struct get_iterator : public Iter {
  typedef typename std::iterator_traits<Iter>::value_type value_type;
  typedef typename std::tuple_element<I, typename std::iterator_traits<Iter>::value_type>::type& reference;
  typedef typename std::tuple_element<I, typename std::iterator_traits<Iter>::value_type>::type *pointer;
  
  get_iterator() {}
  get_iterator(Iter iterator): Iter(iterator) { }
  reference operator*() { return std::get<I>(Iter::operator*()); }
  pointer operator->() { return &get_iterator::operator*(); }
};

template<size_t I, typename Iter>
get_iterator<I, Iter> make_get_iterator(Iter iterator)
{
  return get_iterator<I, Iter>(iterator);
}

using namespace Anki::Cozmo;

const Anki::Comms::ConnectionId ADVERTISING_CLIENT_CONNECTION_ID = 0;

MultiClientComms::MultiClientComms()
{
}

MultiClientComms::~MultiClientComms()
{
}

bool MultiClientComms::IsStarted() const
{
  return _reliableChannel.IsStarted();
}

Anki::Util::TransportAddress MultiClientComms::GetAdvertisingAddress() const
{
  TransportAddress address;
  if (_advertisingChannel.GetAddress(address, ADVERTISING_CLIENT_CONNECTION_ID)) {
    // nothing to do
  }
  return address;
}

void MultiClientComms::Start(const TransportAddress& advertisingAddress)
{
  _advertisingChannel.StartClient();
  _reliableChannel.StartClient();
  
  _advertisingChannel.AddConnection(ADVERTISING_CLIENT_CONNECTION_ID, advertisingAddress);
  SendZeroPacket();
}

void MultiClientComms::Stop()
{
  if (IsStarted()) {
    RemoveAllConnections();
    
    _advertisingChannel.Stop();
    _reliableChannel.Stop();
  }
}

void MultiClientComms::Update()
{
  if (!IsStarted()) {
    return;
  }
  
  double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  
  _advertisingChannel.Update();
  _reliableChannel.Update();
  
  IncomingPacket packet;
  while (_advertisingChannel.PopIncomingPacket(packet)) {
    if (packet.tag == IncomingPacket::Tag::NormalMessage) {
      if (packet.bufferSize == sizeof(Comms::AdvertisementMsg)) {
        Comms::AdvertisementMsg advertisementMessage;
        std::memcpy(&advertisementMessage, packet.buffer, sizeof(Comms::AdvertisementMsg));
        // prevent buffer overruns
        advertisementMessage.ip[sizeof(advertisementMessage.ip) - 1] = 0;
        
        if (!_reliableChannel.IsConnectionActive(advertisementMessage.id)) {
          
          if (_advertisingConnections.count(advertisementMessage.id) == 0) {
            PRINT_STREAM_DEBUG("MultiClientComms.Update",
                                 "Detected advertising connection id " << advertisementMessage.id <<
                              " when hosting on host " << advertisementMessage.ip <<
                              " at port" << advertisementMessage.port << ".");
          }
          _advertisingConnections[advertisementMessage.id] = AdvertisementConnectionInfo(advertisementMessage, currentTime);
        }
      }
    }
  }
  
  auto iterator = _advertisingConnections.begin();
  auto end = _advertisingConnections.end();
  while (iterator != end) {
    if (currentTime - iterator->second.lastSeenTime > ROBOT_ADVERTISING_TIMEOUT_S) {
      PRINT_STREAM_DEBUG("MultiClientComms.Update",
                         "Removing connection id " << iterator->first <<
                         " from advertising list. "
                         " (Last seen: " << iterator->second.lastSeenTime <<
                         ", current time: " << currentTime << ".)");
      _advertisingConnections.erase(iterator);
      // we modified the collection; start over
      iterator = _advertisingConnections.begin();
      end = _advertisingConnections.end();
    }
    else {
      ++iterator;
    }
  }

// TODO: SIMULATE LATENCY
//#if(DO_SIM_COMMS_LATENCY)
//  // Update number of ready to receive messages
//  numRecvRdyMsgs_ = 0;
//  PacketQueue_t::iterator iter;
//  for (iter = recvdMsgPackets_.begin(); iter != recvdMsgPackets_.end(); ++iter) {
//    if (iter->first <= currTime) {
//      ++numRecvRdyMsgs_;
//    } else {
//      break;
//    }
//  }
//  
//  //printf("TIME %f: Total: %d, rel: %d\n", currTime, recvdMsgPackets_.size(), numRecvRdyMsgs_);
//  
//  // Send messages that are scheduled to be sent, up to the outgoing bytes limit.
//  if (send_queued_msgs) {
//    bytesSentThisUpdateCycle_ = 0;
//    std::map<int, PacketQueue_t>::iterator sendQueueIt = sendMsgPackets_.begin();
//    while (sendQueueIt != sendMsgPackets_.end()) {
//      PacketQueue_t* pQueue = &(sendQueueIt->second);
//      while (!pQueue->empty()) {
//        if (pQueue->front().first <= currTime) {
//          
//          if ((maxSentBytesPerTic_ > 0) && (bytesSentThisUpdateCycle_ + pQueue->front().second.dataLen > maxSentBytesPerTic_)) {
//#if(DEBUG_COMMS)
//            PRINT_NAMED_INFO("MultiClientComms.MaxSendLimitReached", "%d messages left in queue to send later\n", pQueue->size() - 1);
//#endif
//            break;
//          }
//          bytesSentThisUpdateCycle_ += pQueue->front().second.dataLen;
//          if (RealSend(pQueue->front().second) < 0) {
//            PRINT_NAMED_WARNING("MultiClientComms.RealSendFail", "");
//          }
//          pQueue->pop_front();
//        } else {
//          break;
//        }
//      }
//      ++sendQueueIt;
//    }
//  }
//#endif  // #if(DO_SIM_COMMS_LATENCY)
  
  // Ping the advertisement channel in case it wasn't present at Init()
  static u8 pingTimer = 0;
  if (pingTimer++ == 10) {
    pingTimer = 0;
    SendZeroPacket();
  }
}

bool MultiClientComms::Send(const Anki::Comms::OutgoingPacket &packet)
{
  return _reliableChannel.Send(packet);
}

// TODO: SIMULATE LATENCY
//  #if(DO_SIM_COMMS_LATENCY)
//  /*
//  // If no send latency, just send now
//  if (SIM_SEND_LATENCY_SEC == 0) {
//    if ((maxSentBytesPerTic_ > 0) && (bytesSentThisUpdateCycle_ + p.dataLen > maxSentBytesPerTic_)) {
//      #if(DEBUG_COMMS)
//      PRINT_NAMED_INFO("MultiClientComms.MaxSendLimitReached", "queueing message\n");
//      #endif
//    } else {
//      bytesSentThisUpdateCycle_ += p.dataLen;
//      return RealSend(p);
//    }
//  }
//  */
//  // Otherwise add to send queue
//  sendMsgPackets_[p.destId].emplace_back(std::piecewise_construct,
//                                         std::forward_as_tuple((f32)(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + SIM_SEND_LATENCY_SEC)),
//                                         std::forward_as_tuple(p));
//  
//  // Fake the number of bytes sent
//  size_t numBytesSent = sizeof(RADIO_PACKET_HEADER) + sizeof(u32) + p.dataLen;
//  return numBytesSent;
//}
//
//int MultiClientComms::RealSend(const Comms::MsgPacket &p)
//{
//  #endif // #if(DO_SIM_COMMS_LATENCY)
//  
//  connectedDevicesIt_t it = connectedDevices_.find(p.destId);
//  if (it != connectedDevices_.end()) {
//    
//    bool isTCP = it->second.protocol == Anki::Comms::TCP;
//    
//    if (isTCP) {
//      // Wrap message in header/footer
//      char sendBuf[p.dataLen + 10]; // Extra bytes for TCP header stuff
//      int sendBufLen = 0;
//      
//      memcpy(sendBuf, RADIO_PACKET_HEADER, sizeof(RADIO_PACKET_HEADER));
//      sendBufLen += sizeof(RADIO_PACKET_HEADER);
//      sendBuf[sendBufLen++] = p.dataLen;
//      sendBuf[sendBufLen++] = p.dataLen >> 8;
//      sendBuf[sendBufLen++] = 0;
//      sendBuf[sendBufLen++] = 0;
//      memcpy(sendBuf + sendBufLen, p.data, p.dataLen);
//      sendBufLen += p.dataLen;
//      return ((TcpClient*)it->second.client)->Send(sendBuf, sendBufLen);
//    } else {
//      return ((UdpClient*)it->second.client)->Send((char*)p.data, p.dataLen);
//    }
//  
//
//    /*
//    printf("SENDBUF (hex): ");
//    PrintBytesHex(sendBuf, sendBufLen);
//    printf("\nSENDBUF (uint): ");
//    PrintBytesUInt(sendBuf, sendBufLen);
//    printf("\n");
//    */
//  }
//  return -1;
//  
//}

bool MultiClientComms::PopIncomingPacket(IncomingPacket& packet)
{
  // technically you really should modify packet if we're going to end up
  // returning false; this just costs some stack and copying
  IncomingPacket bufferPacket;
  
  bool result = _reliableChannel.PopIncomingPacket(bufferPacket);
  if (result) {
    // refuse all incoming connections
    if (packet.tag == IncomingPacket::Tag::ConnectionRequest) {
      _reliableChannel.RefuseIncomingConnection(packet.sourceAddress);
      return false;
    }
    
    packet = bufferPacket;
  }
  return result;
}

bool MultiClientComms::IsConnectionAdvertising(ConnectionId connectionId) const
{
  return (_advertisingConnections.count(connectionId) != 0);
}

bool MultiClientComms::IsConnectionActive(ConnectionId connectionId) const
{
  return _reliableChannel.IsConnectionActive(connectionId);
}

int32_t MultiClientComms::CountAdvertisingConnections() const
{
  return static_cast<int32_t>(_advertisingConnections.size());
}

int32_t MultiClientComms::CountActiveConnections() const
{
  return static_cast<int32_t>(_advertisingConnections.size());
}

bool MultiClientComms::GetAdvertisingConnections(std::vector<ConnectionId>& connectionIds)
{
  connectionIds.assign(make_get_iterator<0>(_advertisingConnections.begin()), make_get_iterator<0>(_advertisingConnections.end()));
  return !connectionIds.empty();
}

void MultiClientComms::AcceptAdvertisingConnection(ConnectionId connectionId)
{
  auto found = _advertisingConnections.find(connectionId);
  if (found != _advertisingConnections.end()) {
    AcceptAdvertisingConnectionInternal(connectionId, found->second);
  }
  else {
    PRINT_STREAM_WARNING("MultiClientComms.AcceptAdvertisingConnection",
                         "Could not accept connection to id " << connectionId <<
                         " because it is not currently in the advertisement list.");
  }
}

void MultiClientComms::AcceptAllAdvertisingConnections()
{
  for (auto entry : _advertisingConnections) {
    AcceptAdvertisingConnectionInternal(entry.first, entry.second);
  }
}

void MultiClientComms::AddConnection(ConnectionId connectionId, const TransportAddress& address)
{
  _reliableChannel.AddConnection(connectionId, address);
}

void MultiClientComms::RemoveAdvertisingConnection(ConnectionId connectionId)
{
  _advertisingConnections.erase(connectionId);
}

void MultiClientComms::RemoveConnection(ConnectionId connectionId)
{
  _reliableChannel.RemoveConnection(connectionId);
}

void MultiClientComms::RemoveAllAdvertisingConnections()
{
  _advertisingConnections.clear();
}

void MultiClientComms::RemoveAllConnections()
{
  _reliableChannel.RemoveAllConnections();
}

bool MultiClientComms::GetConnectionId(ConnectionId& connectionId, const TransportAddress& address) const
{
  return _reliableChannel.GetConnectionId(connectionId, address);
}

bool MultiClientComms::GetAddress(TransportAddress& address, ConnectionId connectionId) const
{
  return _reliableChannel.GetAddress(address, connectionId);
}

uint32_t MultiClientComms::GetMaxTotalBytesPerMessage() const
{
  return _reliableChannel.GetMaxTotalBytesPerMessage();
}

void MultiClientComms::SendZeroPacket()
{
  const uint8_t zero = 0;
  OutgoingPacket packet(&zero, 1, ADVERTISING_CLIENT_CONNECTION_ID, false, true);
  _advertisingChannel.Send(packet);
}

void MultiClientComms::AcceptAdvertisingConnectionInternal(ConnectionId connectionId, const AdvertisementConnectionInfo& info)
{
  const char *ipString = reinterpret_cast<const char *>(info.message.ip);
  uint32_t ipAddress = TransportAddress::IPAddressStringToU32(ipString);
  uint16_t port = info.message.port;
  // won't always happen; sometimes it'll half-parse and give you garbage
  if (ipAddress == 0) {
    PRINT_STREAM_WARNING("MultiClientComms.AcceptAdvertisingConnectionInternal",
                         "Could not accept connection to id " << connectionId <<
                         " because its IP address could not be parsed.");
  }
  else if (port == 0) {
    PRINT_STREAM_WARNING("MultiClientComms.AcceptAdvertisingConnectionInternal",
                         "Could not accept connection to id " << connectionId <<
                         " because its IP address could not be parsed.");
  }
  else {
    TransportAddress address(ipAddress, port);
    
    PRINT_STREAM_DEBUG("MultiClientComms.AcceptAdvertisingConnectionInternal",
                       "Accepting connection id " << connectionId <<
                       " to address " << address.ToString() << ".");
    
    _reliableChannel.AddConnection(connectionId, address);
  }
}
