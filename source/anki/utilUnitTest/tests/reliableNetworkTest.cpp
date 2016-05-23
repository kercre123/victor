/**
 * File: reliableTransportTest
 *
 * Author: Mark Wesley
 * Created: 02/11/15
 *
 * Description: Tests reliable transport communication using a FakeUDPPort
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=ReliableTransportTest*
 **/

#include "util/helpers/includeGTest.h"
#include "util/debug/messageDebugging.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/transport/fakeUDPSocket.h"
#include "util/transport/netEmulatorUDPSocket.h"
#include "util/transport/reliableConnection.h"
#include "util/transport/reliableTransport.h"
#include "util/transport/transportAddress.h"
#include "util/transport/udpTransport.h"
#include <arpa/inet.h>
#include <mutex>
#include "utilUnitTestShared.h"


using namespace Anki::Util;


double GetTimingModeScalar()
{
  double scalar = 1.0;
  
  switch (UtilUnitTestShared::GetTimingMode())
  {
    case UtilUnitTestShared::TimingMode::Valgrind:
      // Scale timeouts etc. up by X to account for valgrind running the tests slower
      scalar = 10.0;
      break;
    default:
      break;
  }
  
  printf("[ReliableTransportTests] Timing Scalar for mode = %f\n", scalar);
  
  return scalar;
}


void InitOverdriveNetSettings()
{
  const uint8_t headerPrefix[] = {'O', 'D', 'R', 1};
  UDPTransport::SetHeaderPrefix(headerPrefix, sizeof(headerPrefix));
  UDPTransport::SetDoesHeaderHaveCRC(true);
  
  ReliableTransport::SetSendAckOnReceipt(true);
  ReliableTransport::SetSendUnreliableMessagesImmediately(true);
  ReliableTransport::SetMaxPacketsToReSendOnAck(1);
  ReliableTransport::SetMaxPacketsToSendOnSendMessage(1);
  
  ReliableConnection::SetSendPacketsImmediately(true);
  ReliableConnection::SetTimeBetweenPingsInMS(250.0);
  ReliableConnection::SetTimeBetweenResendsInMS(50.0);
  ReliableConnection::SetMaxTimeSinceLastSend( ReliableConnection::GetTimeBetweenResendsInMS() - 1.0 );
  ReliableConnection::SetConnectionTimeoutInMS(5000.0 * GetTimingModeScalar());
  ReliableConnection::SetMaxPingRoundTripsToTrack(20);
  ReliableConnection::SetMaxPacketsToReSendOnUpdate(3);
  ReliableConnection::SetMaxBytesFreeInAFullPacket(44);
  ReliableConnection::SetSendSeparatePingMessages(true);
  ReliableConnection::SetPacketSeparationIntervalInMS(0.0);
}


void InitCozmoNetSettings()
{
  const uint8_t headerPrefix[] = {'C', 'O', 'Z', 2};
  UDPTransport::SetHeaderPrefix(headerPrefix, sizeof(headerPrefix));
  UDPTransport::SetDoesHeaderHaveCRC(false);
  
  ReliableTransport::SetSendAckOnReceipt(false);
  ReliableTransport::SetSendUnreliableMessagesImmediately(false);
  ReliableTransport::SetMaxPacketsToReSendOnAck(0);
  ReliableTransport::SetMaxPacketsToSendOnSendMessage(1);
  
  ReliableConnection::SetSendPacketsImmediately(false);
  ReliableConnection::SetTimeBetweenPingsInMS(33.3);
  ReliableConnection::SetTimeBetweenResendsInMS(33.3);
  ReliableConnection::SetMaxTimeSinceLastSend( ReliableConnection::GetTimeBetweenResendsInMS() - 1.0 );
  ReliableConnection::SetConnectionTimeoutInMS(5000.0 * GetTimingModeScalar());
  ReliableConnection::SetMaxPingRoundTripsToTrack(10);
  ReliableConnection::SetMaxPacketsToReSendOnUpdate(1);
  ReliableConnection::SetMaxBytesFreeInAFullPacket(44);
  ReliableConnection::SetSendSeparatePingMessages(false);
  ReliableConnection::SetPacketSeparationIntervalInMS(2.0);
}


void InitNetworkSettings(int configNum)
{
  // try slightly different settings per test to get coverage of different setting combos too
  
  const uint8_t headerPrefix[] = {'T', 'S', 'T', uint8_t(configNum)};
  UDPTransport::SetHeaderPrefix(headerPrefix, sizeof(headerPrefix));
  
  // 0 - crc and ack
  // 1 - ack
  // 2 - crc
  // 3 - neither
  const bool hasCrc  = (configNum == 0) || (configNum == 2);
  const bool sendAck = (configNum == 0) || (configNum == 1);

  UDPTransport::SetDoesHeaderHaveCRC(hasCrc);
  ReliableTransport::SetSendAckOnReceipt(sendAck);
}


class ReliableTransportTest : public ::testing::Test
{
public:
  ReliableTransportTest() {}
};


class TestNetTransportDataReceiver : public Anki::Util::INetTransportDataReceiver
{
public:
  
  TestNetTransportDataReceiver(const char* inName, uint32_t ipAddress)
    : _name(inName)
    , _transport(nullptr)
    , _numRecv(0)
  {
  }
  
  virtual ~TestNetTransportDataReceiver() {}
  
  virtual void ReceiveData(const uint8_t* buffer, uint32_t size, const Anki::Util::TransportAddress& sourceAddress) override
  {
    if (size > 0)
    {
      // Real message (not just connection related)
      
      std::lock_guard<std::mutex> lock(_mutex);
      
      ++_numRecv;
      
      {
        const size_t oldSize = _receivedBytes.size();
        _receivedBytes.resize(oldSize + size);
        memcpy(&_receivedBytes[oldSize], buffer, size);
      }
    }
    else
    {
      std::lock_guard<std::mutex> lock(_mutex);
      
      if (buffer == INetTransportDataReceiver::OnConnectRequest)
      {
        const bool alreadyConnected = HasConnection(sourceAddress);
        if (!alreadyConnected)
        {
          _connections.push_back(sourceAddress);
        }
        printf("[%s] Recv OnConnectRequest from '%s' (%s)\n", _name, sourceAddress.ToString().c_str(), alreadyConnected ? "Dupe" : "New");
        _transport->FinishConnection(sourceAddress);
      }
      else if (buffer == INetTransportDataReceiver::OnConnected)
      {
        const bool alreadyConnected = HasConnection(sourceAddress);
        if (!alreadyConnected)
        {
          _connections.push_back(sourceAddress);
        }
        printf("[%s] Recv OnConnected from '%s' (%s)\n", _name, sourceAddress.ToString().c_str(), alreadyConnected ? "Dupe" : "New");
      }
      else if (buffer == INetTransportDataReceiver::OnDisconnected)
      {
        const int existingConnection = GetConnectionIndex(sourceAddress);
        if (existingConnection >= 0)
        {
          _connections.erase(_connections.begin() + existingConnection);
        }
        printf("[%s] Recv OnDisconnected from '%s' (was index %d)\n", _name, sourceAddress.ToString().c_str(), existingConnection );
      }
      else
      {
        assert(0);
      }
    }
  }
  
  size_t GetNumMessagesReceived() const
  {
    std::lock_guard<std::mutex> lock(_mutex);
    return _numRecv;
  }

  size_t GetNumConnections() const
  {
    std::lock_guard<std::mutex> lock(_mutex);
    return _connections.size();
  }
  
  size_t GetNumBytesReceived() const
  {
    std::lock_guard<std::mutex> lock(_mutex);
    return _receivedBytes.size();
  }

  // you must call Unlock() after you've finished accessing the vector
  const std::vector<uint8_t>& LockAndGetReceivedBytes() const
  {
    _mutex.lock();
    return _receivedBytes;
  }
  
  void Unlock() const
  {
    _mutex.unlock();
  }

  void SetTransport(ReliableTransport* transport)
  {
    _transport = transport;
  }
  
  int GetConnectionIndex(const Anki::Util::TransportAddress& address) const
  {
    int i = 0;
    for (const Anki::Util::TransportAddress& existingAddress : _connections)
    {
      if (existingAddress == address)
      {
        return i;
      }
      else
      {
        ++i;
      }
    }
    
    return -1;
  }
  
  bool HasConnection(const Anki::Util::TransportAddress& address) const
  {
    return (GetConnectionIndex(address) >= 0);
  }
  
private:
  
  const char*           _name;
  ReliableTransport*    _transport;
  size_t                _numRecv;
  std::vector<uint8_t>  _receivedBytes;
  std::vector<Anki::Util::TransportAddress> _connections;
  mutable std::mutex    _mutex;         // so unit tests can safely access data whilst socket thread writes to this
};


struct TestClientWrapper
{
  TestClientWrapper(const char* inName, uint32_t randSeed,  uint32_t ipAddress)
    : _udpTransport( new UDPTransport() )
    , _dataReceiver(inName, ipAddress)
    , _reliableTransport(_udpTransport, &_dataReceiver)
    , _emulatorSocket(randSeed)
    , _fakeSocket(ipAddress)
    , _ipAddress(ipAddress)
  {
    _dataReceiver.SetTransport(&_reliableTransport);
    _emulatorSocket.SetSocketImpl(&_fakeSocket);
    _udpTransport->SetSocketImpl(&_emulatorSocket);
  }
  
  ~TestClientWrapper()
  {
    // Make sure _reliableTransport's thread stops running before anything else is destroyed
    KillThreads();
    delete _udpTransport;
  }
  
  void KillThreads()
  {
    _reliableTransport.KillThread();
  }
  
  TransportAddress GetTransportAddress() const
  {
    return TransportAddress(_ipAddress, _udpTransport->GetPort());
  }
  
  UDPTransport*     _udpTransport;
  TestNetTransportDataReceiver  _dataReceiver;
  ReliableTransport _reliableTransport;
  NetEmulatorUDPSocket _emulatorSocket;
  FakeUDPSocket     _fakeSocket;
  uint32_t          _ipAddress;
};


static uint32_t MakeTestIp(uint32_t num)
{
  assert((num >= 0) && (num <= 255));
  return htonl((192 << 24) | (168 <<  16) | (0 << 8) | (num << 0));
}


// Helper ensures all client threads are killed before it deletes any of them
struct TestClientWrappers3
{
  TestClientWrappers3()
  : _host1  ("Host1",   1, MakeTestIp(1) )
  , _client2("Client2", 2, MakeTestIp(2) )
  , _client3("Client3", 3, MakeTestIp(3) )
  {
  }
  
  ~TestClientWrappers3()
  {
    KillThreads();
  }
  
  void KillThreads()
  {
    _host1.KillThreads();
    _client2.KillThreads();
    _client3.KillThreads();
  }
  
  TestClientWrapper _host1;
  TestClientWrapper _client2;
  TestClientWrapper _client3;
};


bool WaitUntilDoneOrMaxTime(uint32_t sleepPerTickInMS, uint32_t maxDurationInMS, std::function<bool ()> isWorkDoneFunc)
{
  uint32_t timeSlept = 0;
  bool isWorkDone = false;
  while ((timeSlept < maxDurationInMS) && !isWorkDone)
  {
    if (isWorkDoneFunc())
    {
      isWorkDone = true;
    }
    
    usleep(sleepPerTickInMS * 1000);
    timeSlept += sleepPerTickInMS;
  }
  
  //printf("Slept for %u ms - %s\n", timeSlept, isWorkDone ? "Done!" : "TimedOut!");
  
  return isWorkDone;
}


// returns true if receivedBytes is identical to any ordering of all of the sentMessages
bool TestStringsReceivedOKInAnyOrder(const std::vector<uint8_t>& receivedBytes, const std::vector<const char*>& sentMessages)
{
  size_t bytesSent = 0;
  for(const char* messageSent : sentMessages)
  {
    bytesSent += strlen(messageSent);
  }
  
  if (receivedBytes.size() != bytesSent)
  {
    // not a match
    return false;
  }

  {
    std::vector<const char*> messagesLeftToMatch = sentMessages;
    
    size_t receivedOffset = 0; // how far into receivedBytes that we've matched so far
    for (size_t i=0; i < sentMessages.size(); ++i)
    {
      // try to match each message, each time using all unmatched sent messages as candidates
      bool foundMatch = false;
      for (size_t j=0; (!foundMatch && (j < messagesLeftToMatch.size())); ++j)
      {
        const char* messageSent = messagesLeftToMatch[j];
        const size_t messageSentLen = strlen(messageSent);
        
        int bufCompRes = memcmp( &receivedBytes[receivedOffset], messageSent, messageSentLen);
        if (bufCompRes == 0)
        {
          foundMatch = true;
          receivedOffset += messageSentLen;
          messagesLeftToMatch.erase(messagesLeftToMatch.begin() + j);
          
          break;
        }
      }
      
      if (!foundMatch)
      {
        return false;
      }
    }
  }
  
  return true;
}


void TestMultiClientMessageSend(TestClientWrapper& host1, TestClientWrapper& client2, TestClientWrapper& client3, uint32_t maxTimeForTest)
{
  host1._reliableTransport.StartHost();
  client2._reliableTransport.StartClient();
  client3._reliableTransport.StartClient();
  
  const char* k_TestMessage1 = "Host1_To_Host1_TestMessage_1";
  const char* k_TestMessage2 = "Host1_To_Client2_TestMessage_2";
  const char* k_TestMessage3 = "Host1_To_Client3_TestMessage_3";
  const char* k_TestMessage4 = "Client2_To_Host1_TestMessage_4";
  const char* k_TestMessage5 = "Client2_To_Client2_TestMessage_5";
  const char* k_TestMessage6 = "Client2_To_Client3_TestMessage_6";
  const char* k_TestMessage7 = "Client3_To_Host1_TestMessage_7";
  const char* k_TestMessage8 = "Client3_To_Client2_TestMessage_8";
  const char* k_TestMessage9 = "Client3_To_Client3_TestMessage_9";

  // Connect everyone
  host1._reliableTransport.Connect(host1.GetTransportAddress());
  host1._reliableTransport.Connect(client2.GetTransportAddress());
  host1._reliableTransport.Connect(client3.GetTransportAddress());
  client2._reliableTransport.Connect(client2.GetTransportAddress());
  client2._reliableTransport.Connect(client3.GetTransportAddress());
  client3._reliableTransport.Connect(client3.GetTransportAddress());

  WaitUntilDoneOrMaxTime(25, maxTimeForTest, [&] { return ((host1._dataReceiver.GetNumConnections() == 3) &&
                                                           (client2._dataReceiver.GetNumConnections() == 3) &&
                                                           (client3._dataReceiver.GetNumConnections() == 3)); } );

  EXPECT_EQ(host1._dataReceiver.GetNumConnections(), 3);
  EXPECT_EQ(client2._dataReceiver.GetNumConnections(), 3);
  EXPECT_EQ(client3._dataReceiver.GetNumConnections(), 3);

  // Send messages from Host1 to All
  host1._reliableTransport.SendData(true, host1.GetTransportAddress(),   (uint8_t*)k_TestMessage1, uint32_t(strlen(k_TestMessage1)) );
  host1._reliableTransport.SendData(true, client2.GetTransportAddress(), (uint8_t*)k_TestMessage2, uint32_t(strlen(k_TestMessage2)) );
  host1._reliableTransport.SendData(true, client3.GetTransportAddress(), (uint8_t*)k_TestMessage3, uint32_t(strlen(k_TestMessage3)) );
  
  // Send messages from client2 to All
  client2._reliableTransport.SendData(true, host1.GetTransportAddress(),   (uint8_t*)k_TestMessage4, uint32_t(strlen(k_TestMessage4)) );
  client2._reliableTransport.SendData(true, client2.GetTransportAddress(), (uint8_t*)k_TestMessage5, uint32_t(strlen(k_TestMessage5)) );
  client2._reliableTransport.SendData(true, client3.GetTransportAddress(), (uint8_t*)k_TestMessage6, uint32_t(strlen(k_TestMessage6)) );
  
  // Send messages from client3 to All
  client3._reliableTransport.SendData(true, host1.GetTransportAddress(),   (uint8_t*)k_TestMessage7, uint32_t(strlen(k_TestMessage7)) );
  client3._reliableTransport.SendData(true, client2.GetTransportAddress(), (uint8_t*)k_TestMessage8, uint32_t(strlen(k_TestMessage8)) );
  client3._reliableTransport.SendData(true, client3.GetTransportAddress(), (uint8_t*)k_TestMessage9, uint32_t(strlen(k_TestMessage9)) );
  
  WaitUntilDoneOrMaxTime(25, maxTimeForTest, [&] { return ((host1._dataReceiver.GetNumMessagesReceived() >= 3) &&
                                                          (client2._dataReceiver.GetNumMessagesReceived() >= 3) &&
                                                          (client3._dataReceiver.GetNumMessagesReceived() >= 3)); } );
  
  // Verify that all messages arrived correctly
  
  EXPECT_EQ( host1._dataReceiver.GetNumMessagesReceived(), 3 );
  EXPECT_EQ( client2._dataReceiver.GetNumMessagesReceived(), 3 );
  EXPECT_EQ( client3._dataReceiver.GetNumMessagesReceived(), 3 );
  
  {
    std::vector<const char*> sentMessages;
    sentMessages.push_back(k_TestMessage1);
    sentMessages.push_back(k_TestMessage4);
    sentMessages.push_back(k_TestMessage7);
    
    bool res = TestStringsReceivedOKInAnyOrder(host1._dataReceiver.LockAndGetReceivedBytes(), sentMessages);
    host1._dataReceiver.Unlock();
    EXPECT_EQ(res, true);
  }
  {
    std::vector<const char*> sentMessages;
    sentMessages.push_back(k_TestMessage2);
    sentMessages.push_back(k_TestMessage5);
    sentMessages.push_back(k_TestMessage8);
    
    bool res = TestStringsReceivedOKInAnyOrder(client2._dataReceiver.LockAndGetReceivedBytes(), sentMessages);
    client2._dataReceiver.Unlock();
    EXPECT_EQ(res, true);
  }
  {
    std::vector<const char*> sentMessages;
    sentMessages.push_back(k_TestMessage3);
    sentMessages.push_back(k_TestMessage6);
    sentMessages.push_back(k_TestMessage9);
    
    bool res = TestStringsReceivedOKInAnyOrder(client3._dataReceiver.LockAndGetReceivedBytes(), sentMessages);
    client3._dataReceiver.Unlock();
    EXPECT_EQ(res, true);
  }
}


static void MissingPacketTest()
{
  InitNetworkSettings(0);
  
  TestClientWrappers3 testClientWrappers;
  TestClientWrapper& host1   = testClientWrappers._host1;
  TestClientWrapper& client2 = testClientWrappers._client2;
  TestClientWrapper& client3 = testClientWrappers._client3;
  
  // Simulate packet loss on each connection
  const float k_PacketLossPercentage = 50.0f;
  host1._emulatorSocket.SetRandomPacketLossPercentage(k_PacketLossPercentage);
  client2._emulatorSocket.SetRandomPacketLossPercentage(k_PacketLossPercentage);
  client3._emulatorSocket.SetRandomPacketLossPercentage(k_PacketLossPercentage);
  
  const uint32_t k_MaxTimeForTest = 10000; // = 10 seconds
  TestMultiClientMessageSend(host1, client2, client3, k_MaxTimeForTest);
  
  const uint32_t totalPacketsDropped = host1._emulatorSocket.GetNumPacketsDropped() +
                                       client2._emulatorSocket.GetNumPacketsDropped() +
                                       client3._emulatorSocket.GetNumPacketsDropped();
  
  // not a test fail as such, but we want to make sure the deterministic random values we're giving result in at least
  // 1 dropped packet otherwise we weren't really stressing anything
  EXPECT_GE(totalPacketsDropped, 1); // make sure we're dropping enough packets to make the test meaningful
  
  printf("MissingPacketTest: Simulated %u packets dropped total. (host1: %u, client2: %u, client3: %u)\n",
         totalPacketsDropped,
         host1._emulatorSocket.GetNumPacketsDropped(),
         client2._emulatorSocket.GetNumPacketsDropped(),
         client3._emulatorSocket.GetNumPacketsDropped());
  
  const uint32_t outOfOrderHost1   =   host1._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t outOfOrderClient2 = client2._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t outOfOrderClient3 = client3._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t totalOutOfOrder = outOfOrderHost1 + outOfOrderClient2 + outOfOrderClient3;
  
  printf("MissingPacketTest: Simulated %u packets arrived out of order requiring resends. (host1: %u, client2: %u, client3: %u)\n",
         totalOutOfOrder, outOfOrderHost1, outOfOrderClient2, outOfOrderClient3);
}


TEST_F(ReliableTransportTest, MissingPacketTestOverdrive)
{
  InitOverdriveNetSettings();
  MissingPacketTest();
}


TEST_F(ReliableTransportTest, MissingPacketTestCozmo)
{
  InitCozmoNetSettings();
  MissingPacketTest();
}


static void ReorderedJitterTest()
{
  InitNetworkSettings(1);
  
  TestClientWrappers3 testClientWrappers;
  TestClientWrapper& host1   = testClientWrappers._host1;
  TestClientWrapper& client2 = testClientWrappers._client2;
  TestClientWrapper& client3 = testClientWrappers._client3;
  
  // Simulate jitter with wide range of latency for each packet - will cause packet reorder
  const uint32_t k_MinLatency = 3;
  const uint32_t k_MaxLatency = 500;
  host1._emulatorSocket.SetLatencyRangeInMS(k_MinLatency, k_MaxLatency);
  client2._emulatorSocket.SetLatencyRangeInMS(k_MinLatency, k_MaxLatency);
  client3._emulatorSocket.SetLatencyRangeInMS(k_MinLatency, k_MaxLatency);
  
  const uint32_t k_MaxTimeForTest = 10000; // = 10 seconds
  TestMultiClientMessageSend(host1, client2, client3, k_MaxTimeForTest);
  
  const uint32_t totalPacketsDropped = host1._emulatorSocket.GetNumPacketsDropped() +
                                       client2._emulatorSocket.GetNumPacketsDropped() +
                                       client3._emulatorSocket.GetNumPacketsDropped();
  
  printf("ReorderedJitterTest: Simulated %u packets dropped total. (host1: %u, client2: %u, client3: %u)\n",
           totalPacketsDropped,
           host1._emulatorSocket.GetNumPacketsDropped(),
           client2._emulatorSocket.GetNumPacketsDropped(),
           client3._emulatorSocket.GetNumPacketsDropped());
  
  const uint32_t outOfOrderHost1   =   host1._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t outOfOrderClient2 = client2._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t outOfOrderClient3 = client3._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t totalOutOfOrder = outOfOrderHost1 + outOfOrderClient2 + outOfOrderClient3;
  
  printf("ReorderedJitterTest: Simulated %u packets arrived out of order requiring resends. (host1: %u, client2: %u, client3: %u)\n",
         totalOutOfOrder, outOfOrderHost1, outOfOrderClient2, outOfOrderClient3);
  
  // not a test fail as such, but we want to make sure the deterministic random values we're giving result in at least
  // 1 out of order message otherwise we weren't really stressing anything
  EXPECT_GE( totalOutOfOrder, 1 );
}


TEST_F(ReliableTransportTest, ReorderedJitterTestOverdrive)
{
  InitOverdriveNetSettings();
  ReorderedJitterTest();
}


TEST_F(ReliableTransportTest, ReorderedJitterTestCozmo)
{
  InitCozmoNetSettings();
  ReorderedJitterTest();
}


static void HugePacketTest()
{
  // Send a huge packet that must be reassembled on the other end
  
  InitNetworkSettings(2);
  
  TestClientWrappers3 testClientWrappers;
  TestClientWrapper& host1   = testClientWrappers._host1;
  TestClientWrapper& client2 = testClientWrappers._client2;
  
  const uint32_t k_MinLatency = 3;
  const uint32_t k_MaxLatency = 200;
  const float k_PacketLossPercentage = 25.0f;
  host1._emulatorSocket.SetLatencyRangeInMS(k_MinLatency, k_MaxLatency);
  client2._emulatorSocket.SetLatencyRangeInMS(k_MinLatency, k_MaxLatency);
  host1._emulatorSocket.SetRandomPacketLossPercentage(k_PacketLossPercentage);
  client2._emulatorSocket.SetRandomPacketLossPercentage(k_PacketLossPercentage);
  
  host1._reliableTransport.StartHost();
  client2._reliableTransport.StartClient();
  
  const uint32_t k_MaxTimeForTest = 10000; // = 10 seconds

  host1._reliableTransport.Connect(client2.GetTransportAddress());
  WaitUntilDoneOrMaxTime(25, k_MaxTimeForTest, [&] { return (host1._dataReceiver.GetNumConnections() == 1) && (client2._dataReceiver.GetNumConnections() == 1); } );

  const size_t k_NumUIntsToStore = 1024 * 2; // 2K ints -> 8KB message
  uint32_t* hugeMessageBuffer = new uint32_t[k_NumUIntsToStore];
  const size_t k_SizeOfHugeMessageBuffer = sizeof(hugeMessageBuffer[0]) * k_NumUIntsToStore;

  for (uint32_t i=0; i < k_NumUIntsToStore; ++i)
  {
    hugeMessageBuffer[i] = i;
  }
  
  host1._reliableTransport.SendData(true, client2.GetTransportAddress(), reinterpret_cast<uint8_t*>(hugeMessageBuffer), k_SizeOfHugeMessageBuffer);

  WaitUntilDoneOrMaxTime(25, k_MaxTimeForTest, [&] { return (client2._dataReceiver.GetNumBytesReceived() >= k_SizeOfHugeMessageBuffer); } );
  
  EXPECT_EQ( client2._dataReceiver.GetNumBytesReceived(), k_SizeOfHugeMessageBuffer );
  
  if (client2._dataReceiver.GetNumBytesReceived() == k_SizeOfHugeMessageBuffer)
  {
    const int bufCompRes = memcmp( &client2._dataReceiver.LockAndGetReceivedBytes()[0], hugeMessageBuffer, k_SizeOfHugeMessageBuffer);
    client2._dataReceiver.Unlock();
    EXPECT_EQ( bufCompRes, 0 );
  }
  
  delete[] hugeMessageBuffer;
  hugeMessageBuffer = nullptr;
 
  const uint32_t totalPacketsDropped = host1._emulatorSocket.GetNumPacketsDropped() + client2._emulatorSocket.GetNumPacketsDropped();
  
  // not a test fail as such, but we want to make sure the deterministic random values we're giving result in at least
  // 1 dropped packet otherwise we weren't really stressing anything
  EXPECT_GE(totalPacketsDropped, 1);
  
  printf("HugePacketTest: Simulated %u packets dropped total. (host1: %u, client2: %u)\n",
         totalPacketsDropped,
         host1._emulatorSocket.GetNumPacketsDropped(),
         client2._emulatorSocket.GetNumPacketsDropped());
  
  const uint32_t outOfOrderHost1   =   host1._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t outOfOrderClient2 = client2._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t totalOutOfOrder = outOfOrderHost1 + outOfOrderClient2;
  
  printf("HugePacketTest: Simulated %u packets arrived out of order requiring resends. (host1: %u, client2: %u)\n",
         totalOutOfOrder, outOfOrderHost1, outOfOrderClient2);
  
  // not a test fail as such, but we want to make sure the deterministic random values we're giving result in at least
  // 1 out of order message (from packet lost or reorder) otherwise we weren't really stressing anything
  EXPECT_GE( totalOutOfOrder, 1 );
}


TEST_F(ReliableTransportTest, HugePacketTestOverdrive)
{
  InitOverdriveNetSettings();
  HugePacketTest();
}


TEST_F(ReliableTransportTest, HugePacketTestCozmo)
{
  InitCozmoNetSettings();
  HugePacketTest();
}


static void MultipleLargePacketTest()
{
  // Like HugePacketTest, but splits sends into multiple large packets that are small enough to fit several (~5) in
  // 1 UDP packet, but are large and numerous enough that it will take multiple large UDP packets to fit all of them.
  // Verifies in-order ness of discrete messages and underlying reliable code that puts as many reliable messages
  // into 1 UDP packet as it can
  
  InitNetworkSettings(3);
  
  TestClientWrappers3 testClientWrappers;
  TestClientWrapper& host1   = testClientWrappers._host1;
  TestClientWrapper& client2 = testClientWrappers._client2;
  
  const uint32_t k_MinLatency = 3;
  const uint32_t k_MaxLatency = 200;
  const float k_PacketLossPercentage = 25.0f;
  host1._emulatorSocket.SetLatencyRangeInMS(k_MinLatency, k_MaxLatency);
  client2._emulatorSocket.SetLatencyRangeInMS(k_MinLatency, k_MaxLatency);
  host1._emulatorSocket.SetRandomPacketLossPercentage(k_PacketLossPercentage);
  client2._emulatorSocket.SetRandomPacketLossPercentage(k_PacketLossPercentage);
  
  host1._reliableTransport.StartHost();
  client2._reliableTransport.StartClient();
  
  const uint32_t k_MaxTimeForTest = 10000; // = 10 seconds

  host1._reliableTransport.Connect(client2.GetTransportAddress());
  WaitUntilDoneOrMaxTime(25, k_MaxTimeForTest, [&] { return (host1._dataReceiver.GetNumConnections() == 1) && (client2._dataReceiver.GetNumConnections() == 1); } );

  const size_t k_NumUIntsToStore = 1024 * 2; // 2K ints -> 8KB message
  uint32_t* hugeMessageBuffer = new uint32_t[k_NumUIntsToStore];
  const size_t k_SizeOfHugeMessageBuffer = sizeof(hugeMessageBuffer[0]) * k_NumUIntsToStore;
  
  for (uint32_t i=0; i < k_NumUIntsToStore; ++i)
  {
    hugeMessageBuffer[i] = i;
  }
  
  // Multiple smaller sends of k_BytesPerMessage to transmit data
  {
    const size_t k_BytesPerMessage = 250;
    size_t numBytesSent = 0;
    const uint8_t* messageBytes = reinterpret_cast<const uint8_t*>(hugeMessageBuffer);
    while (numBytesSent < k_SizeOfHugeMessageBuffer)
    {
      const uint32_t bytesLeftToSend = Anki::Util::numeric_cast<uint32_t>(k_SizeOfHugeMessageBuffer - numBytesSent);
      const uint32_t bytesToSend = (bytesLeftToSend < k_BytesPerMessage) ? bytesLeftToSend : k_BytesPerMessage;
      host1._reliableTransport.SendData(true, client2.GetTransportAddress(), &messageBytes[numBytesSent], bytesToSend);
      numBytesSent += bytesToSend;
    }
  }

  WaitUntilDoneOrMaxTime(25, k_MaxTimeForTest, [&] { return (client2._dataReceiver.GetNumBytesReceived() >= k_SizeOfHugeMessageBuffer); } );
  
  EXPECT_EQ( client2._dataReceiver.GetNumBytesReceived(), k_SizeOfHugeMessageBuffer );
  
  if (client2._dataReceiver.GetNumBytesReceived() == k_SizeOfHugeMessageBuffer)
  {
    const int bufCompRes = memcmp( &client2._dataReceiver.LockAndGetReceivedBytes()[0], hugeMessageBuffer, k_SizeOfHugeMessageBuffer);
    client2._dataReceiver.Unlock();
    EXPECT_EQ( bufCompRes, 0 );
  }
  
  delete[] hugeMessageBuffer;
  hugeMessageBuffer = nullptr;
  
  const uint32_t totalPacketsDropped = host1._emulatorSocket.GetNumPacketsDropped() + client2._emulatorSocket.GetNumPacketsDropped();
  
  // not a test fail as such, but we want to make sure the deterministic random values we're giving result in at least
  // 1 dropped packet otherwise we weren't really stressing anything
  EXPECT_GE(totalPacketsDropped, 1);
  
  printf("MultipleLargePacketTest: Simulated %u packets dropped total. (host1: %u, client2: %u)\n",
         totalPacketsDropped,
         host1._emulatorSocket.GetNumPacketsDropped(),
         client2._emulatorSocket.GetNumPacketsDropped());
  
  const uint32_t outOfOrderHost1   =   host1._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t outOfOrderClient2 = client2._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t totalOutOfOrder = outOfOrderHost1 + outOfOrderClient2;
  
  printf("MultipleLargePacketTest: Simulated %u packets arrived out of order requiring resends. (host1: %u, client2: %u)\n",
         totalOutOfOrder, outOfOrderHost1, outOfOrderClient2);
  
  // not a test fail as such, but we want to make sure the deterministic random values we're giving result in at least
  // 1 out of order message (from packet lost or reorder) otherwise we weren't really stressing anything
  EXPECT_GE( totalOutOfOrder, 1 );
}


TEST_F(ReliableTransportTest, MultipleLargePacketTestOverdrive)
{
  InitOverdriveNetSettings();
  MultipleLargePacketTest();
}


TEST_F(ReliableTransportTest, MultipleLargePacketTestCozmo)
{
  InitCozmoNetSettings();
  MultipleLargePacketTest();
}


static void MixUnreliableAndReliableTest()
{
  // Like MultipleLargePacketTest, but mix in some unreliable messages
  // unreliable messages may not arrive...
  
  TestClientWrappers3 testClientWrappers;
  TestClientWrapper& host1   = testClientWrappers._host1;
  TestClientWrapper& client2 = testClientWrappers._client2;
  
  const uint32_t k_MinLatency = 3;
  const uint32_t k_MaxLatency = 200;
  const float k_PacketLossPercentage = 12.0f;
  host1._emulatorSocket.SetLatencyRangeInMS(k_MinLatency, k_MaxLatency);
  client2._emulatorSocket.SetLatencyRangeInMS(k_MinLatency, k_MaxLatency);
  host1._emulatorSocket.SetRandomPacketLossPercentage(k_PacketLossPercentage);
  client2._emulatorSocket.SetRandomPacketLossPercentage(k_PacketLossPercentage);
  
  host1._reliableTransport.StartHost();
  client2._reliableTransport.StartClient();
  
  const uint32_t k_MaxTimeForTest = 10000; // = 10 seconds
  
  host1._reliableTransport.Connect(client2.GetTransportAddress());
  WaitUntilDoneOrMaxTime(25, k_MaxTimeForTest, [&] { return (host1._dataReceiver.GetNumConnections() == 1) && (client2._dataReceiver.GetNumConnections() == 1); } );
  
  const size_t k_NumUIntsToStore = 1024 * 2; // 2K ints -> 8KB message
  uint32_t* hugeMessageBuffer = new uint32_t[k_NumUIntsToStore];
  const size_t k_SizeOfHugeMessageBuffer = sizeof(hugeMessageBuffer[0]) * k_NumUIntsToStore;
  
  for (uint32_t i=0; i < k_NumUIntsToStore; ++i)
  {
    hugeMessageBuffer[i] = i;
  }
  
  const size_t k_NumUnreliableBytes = 6;
  uint8_t smallUnreliableMessage[k_NumUnreliableBytes] = {'U', 'N', 'R', 'E', 'L', 'T'};

  // Multiple smaller sends of k_BytesPerMessage to transmit data
  
  size_t numUnreliableSends = 0;
  const size_t k_BytesPerMessage = 250;
  // must send less unreliable bytes than 1 reliable message to ensure we don't break the wait until all reliable have arrived
  const size_t maxUnreliableSends = (k_BytesPerMessage / k_NumUnreliableBytes) - 1;
  {
    
    size_t numBytesSent = 0;
    const uint8_t* messageBytes = reinterpret_cast<const uint8_t*>(hugeMessageBuffer);
    while (numBytesSent < k_SizeOfHugeMessageBuffer)
    {
      const uint32_t bytesLeftToSend = Anki::Util::numeric_cast<uint32_t>(k_SizeOfHugeMessageBuffer - numBytesSent);
      const uint32_t bytesToSend = (bytesLeftToSend < k_BytesPerMessage) ? bytesLeftToSend : k_BytesPerMessage;
      host1._reliableTransport.SendData(true, client2.GetTransportAddress(), &messageBytes[numBytesSent], bytesToSend);
            numBytesSent += bytesToSend;
      
      if (numUnreliableSends < maxUnreliableSends)
      {
        host1._reliableTransport.SendData(false, client2.GetTransportAddress(), smallUnreliableMessage, sizeof(smallUnreliableMessage));
        ++numUnreliableSends;
      }
    }
  }
  
  WaitUntilDoneOrMaxTime(25, k_MaxTimeForTest, [&] { return (client2._dataReceiver.GetNumBytesReceived() >= k_SizeOfHugeMessageBuffer); } );
  
  ASSERT_GT( client2._dataReceiver.GetNumBytesReceived(), k_SizeOfHugeMessageBuffer - 1);
  
  const size_t extraBytesReceived = client2._dataReceiver.GetNumBytesReceived() - k_SizeOfHugeMessageBuffer;
  const size_t numUnreliableReceived = extraBytesReceived / k_NumUnreliableBytes;
  
  // can't guarentee any unreliable messages are received given random packet loss (especially with cozmo settings that tend to put them all in 1 packet)
  //ASSERT_GT( numUnreliableReceived, 0 ); // need at least some unreliable messages to make it through...
  ASSERT_EQ( numUnreliableReceived * k_NumUnreliableBytes, extraBytesReceived ); // should be exact multiple
  
  if (client2._dataReceiver.GetNumBytesReceived() >= k_SizeOfHugeMessageBuffer)
  {
    const std::vector<uint8_t>& clientBytes = client2._dataReceiver.LockAndGetReceivedBytes();
    const uint8_t* expectedBytes = reinterpret_cast<const uint8_t*>(hugeMessageBuffer);
    
    size_t cI = 0;
    size_t bI = 0;
    size_t numRelBytesMatched = 0;
    while (bI < k_SizeOfHugeMessageBuffer)
    {
      bool matchedUnreliableSeq = true;
      size_t uI = 0;
      while (uI < k_NumUnreliableBytes)
      {
        if (smallUnreliableMessage[uI] != clientBytes[cI + uI])
        {
          // not a match for the entire unreliable sequence
          matchedUnreliableSeq = false;
          break;
        }
        else
        {
          ++uI;
        }
      }
      
      if (matchedUnreliableSeq)
      {
        cI += k_NumUnreliableBytes;
      }
      else
      {
        if (clientBytes[cI] != expectedBytes[bI])
        {
          ASSERT_EQ(clientBytes[cI], expectedBytes[bI]);
        }
        else
        {
          ++numRelBytesMatched;
        }
        ++cI;
        ++bI;
      }
    }
    
    ASSERT_EQ(numRelBytesMatched, k_SizeOfHugeMessageBuffer);
    
    client2._dataReceiver.Unlock();
  }
  
  delete[] hugeMessageBuffer;
  hugeMessageBuffer = nullptr;
  
  const uint32_t totalPacketsDropped = host1._emulatorSocket.GetNumPacketsDropped() + client2._emulatorSocket.GetNumPacketsDropped();
  
  // not a test fail as such, but we want to make sure the deterministic random values we're giving result in at least
  // 1 dropped packet otherwise we weren't really stressing anything
  EXPECT_GE(totalPacketsDropped, 1);
  
  printf("MixUnreliableAndReliableTest: Simulated %u packets dropped total. (host1: %u, client2: %u)\n",
         totalPacketsDropped,
         host1._emulatorSocket.GetNumPacketsDropped(),
         client2._emulatorSocket.GetNumPacketsDropped());
  
  const uint32_t outOfOrderHost1   =   host1._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t outOfOrderClient2 = client2._reliableTransport.GetTransportStats().GetRecvStats().GetErrorCount(eME_OutOfOrder);
  const uint32_t totalOutOfOrder = outOfOrderHost1 + outOfOrderClient2;
  
  printf("MixUnreliableAndReliableTest: Simulated %u packets arrived out of order requiring resends. (host1: %u, client2: %u). %zu unreliable arrived out of %zu\n",
         totalOutOfOrder, outOfOrderHost1, outOfOrderClient2, numUnreliableReceived, numUnreliableSends);
  
  // not a test fail as such, but we want to make sure the deterministic random values we're giving result in at least
  // 1 out of order message (from packet lost or reorder) otherwise we weren't really stressing anything
  EXPECT_GE( totalOutOfOrder, 1 );
}


TEST_F(ReliableTransportTest, MixUnreliableAndReliableOverdrive)
{
  InitOverdriveNetSettings();
  MixUnreliableAndReliableTest();
}


TEST_F(ReliableTransportTest, MixUnreliableAndReliableCozmo)
{
  InitCozmoNetSettings();
  MixUnreliableAndReliableTest();
}
