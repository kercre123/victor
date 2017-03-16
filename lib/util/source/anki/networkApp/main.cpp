//
//  main.cpp
//  networkTestApp
//
//  Created by Mark Wesley on 1/28/15.
//  Copyright (c) 2015 Anki. All rights reserved.
//


#include <iostream>
#include "util/debug/messageDebugging.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/transport/fakeUDPSocket.h"
#include "util/transport/netEmulatorUDPSocket.h"
#include "util/transport/reliableTransport.h"
#include "util/transport/transportAddress.h"
#include "util/transport/udpTransport.h"
#include <unistd.h>

        
struct ConfigSettings
{
  ConfigSettings()
  : _ipAddress("127.0.0.1")
  , _port(12345)
  , _numSends(0)
  , _freqSends(20)    // every 20 sleeps = every 20ms
  , _sleepInUs(1000)  // every 1ms
  , _isVerbose(true)
  , _sendReliable(true)
  , _overridePort(true)
  , _printMessagesReceived(true)
  , _useFakeUDPSocket(false)
  , _fakeUDPPacketLossPercentage(90.0f)
  {
    
  }
  
  std::string _ipAddress;
  int         _port;
  int         _numSends;
  int         _freqSends;
  uint32_t    _sleepInUs; // microseconds
  bool        _isVerbose;
  bool        _sendReliable;
  bool        _overridePort;
  bool        _printMessagesReceived;
  bool        _useFakeUDPSocket;
  float       _fakeUDPPacketLossPercentage;
};


ConfigSettings gConfigSettings;


bool ParseArgs(int argc, const char * argv[], ConfigSettings& configSettings)
{
  int argIndex = 1;
  while (argIndex < argc)
  {
    bool handledArg = false;
    
    const char* argName = argv[argIndex];
    if (argName && (argName[0] == '-'))
    {
      if (strcmp(&argName[1], "ip") == 0)
      {
        ++argIndex;
        if (argIndex < argc)
        {
          configSettings._ipAddress = argv[argIndex];
          handledArg = true;
        }
        else
        {
          printf("Error: Argument %s requires an ip address\n", argName);
          return false;
        }
      }
      else if (strcmp(&argName[1], "port") == 0)
      {
        ++argIndex;
        if (argIndex < argc)
        {
          configSettings._port = atoi(argv[argIndex]);
          configSettings._overridePort = true;
          handledArg = true;
        }
        else
        {
          printf("Error: Argument %s requires a port number\n", argName);
          return false;
        }
      }
      else if (strcmp(&argName[1], "send") == 0)
      {
        ++argIndex;
        if (argIndex < argc)
        {
          configSettings._numSends = atoi(argv[argIndex]);
          
          printf("Send Mode %s - will send %d messages\n", (configSettings._numSends > 0) ? "On" : "Off", configSettings._numSends);
          handledArg = true;
        }
        else
        {
          printf("Error: Argument %s requires a number\n", argName);
          return false;
        }
      }
      else if (strcmp(&argName[1], "freq") == 0)
      {
        ++argIndex;
        if (argIndex < argc)
        {
          configSettings._freqSends = atoi(argv[argIndex]);
          
          printf("If Send Mode is on, will send messages every %d ticks\n", configSettings._freqSends);
          handledArg = true;
        }
        else
        {
          printf("Error: Argument %s requires a number\n", argName);
          return false;
        }
      }
      else if (strcmp(&argName[1], "rel") == 0)
      {
        ++argIndex;
        if (argIndex < argc)
        {
          configSettings._sendReliable = (atoi(argv[argIndex]) != 0);
          
          printf("Will send messages as %seliable\n", configSettings._sendReliable ? "R" : "Unr");
          handledArg = true;
        }
        else
        {
          printf("Error: Argument %s requires a number\n", argName);
          return false;
        }
      }
      else if (strcmp(&argName[1], "sleep") == 0)
      {
        ++argIndex;
        if (argIndex < argc)
        {
          configSettings._sleepInUs = (atoi(argv[argIndex]) != 0);
          
          printf("Will sleep for %u microseconds per tick\n", configSettings._sleepInUs);
          handledArg = true;
        }
        else
        {
          printf("Error: Argument %s requires a number\n", argName);
          return false;
        }
      }
      else if (strcmp(&argName[1], "fakeUDP") == 0)
      {
        ++argIndex;
        if (argIndex < argc)
        {
          configSettings._useFakeUDPSocket = (atoi(argv[argIndex]) != 0);
          
          printf("Will use %s UDP Socket\n", configSettings._useFakeUDPSocket ? "Fake" : "Real");
          handledArg = true;
        }
        else
        {
          printf("Error: Argument %s requires a number\n", argName);
          return false;
        }
      }
      else if (strcmp(&argName[1], "fakeRandomLoss") == 0)
      {
        ++argIndex;
        if (argIndex < argc)
        {
          configSettings._fakeUDPPacketLossPercentage = (atof(argv[argIndex]) != 0);
          
          printf("FakeUDP PacketLossPercentage = %2.2f\n", configSettings._fakeUDPPacketLossPercentage);
          handledArg = true;
        }
        else
        {
          printf("Error: Argument %s requires a number\n", argName);
          return false;
        }
      }
      else if ((strcmp(&argName[1], "v") == 0) || (strcmp(&argName[1], "verbose") == 0))
      {
        printf("Verbose Mode On!\n");
        configSettings._isVerbose = true;
        handledArg = true;
      }
    }
    else
    {
      // not a "-" option!?
    }
    
    if (!handledArg)
    {
      printf("Error: Unknown argument '%s'\n", argName);
      return false;
    }
    
    ++ argIndex;
  }
  
  return true;
}


//                                               1         2         3         4
//                                      1234567890123456789012345678901234567890
static const char* k_MessagePreamble = "MESSAGE_10_ABCDEF_20_GHIJKL_30_MNOPQR_40_num_";


class DummyNetTransportDataReceiver : public Anki::Util::INetTransportDataReceiver
{
public:
  DummyNetTransportDataReceiver()
    : _numRecv(0)
    , _numBytesRecv(0)
  {
  }
  
  virtual ~DummyNetTransportDataReceiver() {}
  
  virtual void ReceiveData(const uint8_t* buffer, uint32_t size, const Anki::Util::TransportAddress& sourceAddress) override
  {
    ++_numRecv;
    _numBytesRecv += size;
   
    if (size > 0)
    {
      if (gConfigSettings._printMessagesReceived)
      {
        printf("[NetworkTestApp] Received %u bytes from %s:\n", size, sourceAddress.ToString().c_str() );
        Anki::Util::LogVerboseMessage("AppRecv", "", buffer, size, nullptr);
      }
    }
  }
  
  size_t _numRecv;
  size_t _numBytesRecv;
};


int main(int argc, const char * argv[])
{
  using namespace Anki::Util;

  PrintfLoggerProvider printfLoggerProvider;
  printfLoggerProvider.SetMinLogLevel(Anki::Util::ILoggerProvider::LOG_LEVEL_DEBUG);
  gLoggerProvider = &printfLoggerProvider;

  if (!ParseArgs(argc, argv, gConfigSettings))
  {
    printf( "Usage: %s [-ip IP_ADDRESS] [-port PORT_NUMBER] [-send NUM_MESSAGES] [-freq TICKS_PER_MSG] [-rel 1/0] [-fakeUDP 1/0] [-fakeRandomLoss xx.x%%][-sleep MicroSec] [-v]\n", argv[0] );
    return EXIT_SUCCESS;
  }
  
  DummyNetTransportDataReceiver dataReceiver;
  
  UDPTransport udpTransport;
  ReliableTransport reliableTransport(&udpTransport, &dataReceiver);
  if (gConfigSettings._overridePort)
  {
    udpTransport.SetPort(gConfigSettings._port);
  }
  

  NetEmulatorUDPSocket  netEmulatorUDPSocket(1);
  FakeUDPSocket fakeUDPSocket;
  netEmulatorUDPSocket.SetRandomPacketLossPercentage(gConfigSettings._fakeUDPPacketLossPercentage);
  if (gConfigSettings._useFakeUDPSocket)
  {
    netEmulatorUDPSocket.SetSocketImpl(&fakeUDPSocket);
    udpTransport.SetSocketImpl(&netEmulatorUDPSocket);
  }
  reliableTransport.StartHost();
  int activePort = udpTransport.GetPort();
  
  // if freqSends is large make sure first send is fairly soon
  const int k_MaxTicksTilFirstSend = 10;
  int tickCount = (gConfigSettings._freqSends > k_MaxTicksTilFirstSend) ? (gConfigSettings._freqSends - k_MaxTicksTilFirstSend) : 0;
  int numSent   = 0;
  
  uint32_t sendIpAddress;

  {
    TransportAddress destAddress;
    destAddress.SetIPAddressFromString(gConfigSettings._ipAddress.c_str(), activePort);

    sendIpAddress = destAddress.GetIPAddress();
    printf("[NetworkTestApp] sendIpAddress = 0x%08x from '%s'\n", sendIpAddress, gConfigSettings._ipAddress.c_str());

    reliableTransport.Connect(destAddress);
  }
  
  while (udpTransport.IsConnected())
  {
    reliableTransport.Update();
    
    if (gConfigSettings._numSends > 0)
    {
      // Throttle the frequency that we're sending messages
      
      ++tickCount;
      if (tickCount > gConfigSettings._freqSends)
      {
        tickCount = 0;
        
        if (numSent < gConfigSettings._numSends)
        {
          char buffer[256];
          
          snprintf(buffer, sizeof(buffer), "%s%d", k_MessagePreamble, numSent);
          printf("[NetworkTestApp] Sending '%s'\n", buffer);
          TransportAddress destAddress;
          destAddress.SetIPAddress(sendIpAddress, activePort);
   
          reliableTransport.SendData(gConfigSettings._sendReliable, destAddress, (uint8_t*)buffer, uint32_t(strlen(buffer)) );
          ++numSent;
        }
        
      }
    }
    
    
    usleep(gConfigSettings._sleepInUs);
  }
  
  return EXIT_SUCCESS;
}
