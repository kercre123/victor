/**
 * File: commsLogReader.cpp
 *
 * Author: Damjan Stulic
 * Created: 8/17/2012
 *
 * Description: Reads received messages from a file, and passes them on
 *
 * Copyright: Anki, Inc. 2012
 *
 **/
#ifdef COZMO_RECORDING_PLAYBACK

#include "commsPlayback.h"
#include "coretech/common/engine/utils/timer.h"
//#include "basestation/utils/debug.h"
#include "engine/utils/exceptions.h"
#include "coretech/common/engine/utils/logging/logging.h"

//using namespace AnkiUtil;

namespace Anki {
namespace Vector {


// Constructor
CommsPlayback::CommsPlayback(string & fileName, Comms::IComms *realComms)
: CommsRecorder(fileName, realComms)
{
}

// Destructor
CommsPlayback::~CommsPlayback()
{
  unsigned int size = (unsigned int)dataFromFile.size();
  for (unsigned int i = 0; i < size; ++i)
    delete dataFromFile[i].buffer;
  dataFromFile.clear();
}

//////////////////////////
// ICOMMS implementation

bool CommsPlayback::IsInitialized()
{
  return true;
}

u32 CommsPlayback::GetNumPendingMsgPackets()
{
  unsigned long long currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  int numPending = 0;
  unsigned int size = (unsigned int)dataFromFile.size();
  // count number of messges for this tick
  for (unsigned int i = nextValue; i < size; ++i) {
    // dump all messages that happened in the past
    if (dataFromFile[i].time <= currentTime)
      numPending++;
    else
      break;
  }
#ifdef DEBUG_RECORDING_PLAYBACK
  if (numPending > 0)
    PRINT_NAMED_INFO("CommsPlayback", "messages pending %d", numPending);
#endif
  return numPending;
}


bool CommsPlayback::GetNextMsgPacket(Comms::MsgPacket &msg)
{
  if (GetNumPendingMsgPackets() > 0) {
  
    #ifdef DEBUG_RECORDING_PLAYBACK
    printf("SENDING %d ", nextValue);
    dataFromFile[nextValue].Print();
    #endif
    msg.sourceId = dataFromFile[nextValue].sourceVehicleId;
    msg.dataLen = dataFromFile[nextValue].bufferSize;
    msg.timeStamp = dataFromFile[nextValue].timeStamp;
    memset(msg.data, 0, MAX_BLE_MSG_SIZE);
    memcpy(msg.data, dataFromFile[nextValue].buffer, msg.dataLen);
    #ifdef DEBUG_RECORDING_PLAYBACK
    PrintfCharArray(bleBuffer, MAX_BLE_MSG_SIZE);
    printf("\n\n");
    #endif
    nextValue++;
    return true;
  }
  
  return false;
}

size_t CommsPlayback::Send(const Comms::MsgPacket &msg)
{
  // noop
  size_t numBytesSent = 6 + msg.dataLen; // It's actually sizeof(RADIO_PACKET_HEADER) + sizeof(u32) + msg.dataLen but nobody really cares about this anyway.
  
  // Increment the number of sent messages for the specified destination device
  ++numSendMsgs_[msg.destId];
  
  return numBytesSent;
}
  
u32 CommsPlayback::GetNumMsgPacketsInSendQueue(int devID) {
  std::map<int, int>::iterator it = numSendMsgs_.find(devID);
  if (it != numSendMsgs_.end()) {
    return it->second;
  }
  return 0;
}
  
void CommsPlayback::Update(bool send_queued_msgs) {
  numSendMsgs_.clear();
}
  
// ICOMMS implementation
//////////////////////////


// Reads all data from file into internal data storage
void CommsPlayback::ReadFromFile()
{
  //read from file
  ifstream logFileStream (fileName_.c_str(), ifstream::binary);
  logFileStream.seekg(0, ifstream::end);
  int iSize = (int)logFileStream.tellg();
  if(iSize == -1) {
    PRINT_NAMED_ERROR("CommsPlayback.FileError","tellg returned -1 for file '%s'", fileName_.c_str());
    throw ET_INVALID_FILE;
  }
  unsigned long int size = (unsigned long int)iSize;
  logFileStream.seekg(0);
  unsigned char buffer[size];
  logFileStream.read((char*)&buffer, size);
  logFileStream.close();
  timeLastMessageRecorded = ReadFromBuffer(buffer, (unsigned int)size, dataFromFile);
  printf("COMMS msg count read from file %d\n", (int)dataFromFile.size());
  nextValue = 0;
}

// Called when it is appropriate to do large amount of io
void CommsPlayback::PrepareLogFile()
{
  ReadFromFile();
}

} // end namespace Vector
} // end namespace Anki

#endif
