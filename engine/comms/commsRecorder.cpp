/**
 * File: commsLogWriter.cpp
 *
 * Author: Damjan Stulic
 * Created: 8/17/2012
 *
 * Description: Writes received messages to a file, and passes them on
 *
 * Copyright: Anki, Inc. 2012
 *
 **/
#ifdef COZMO_RECORDING_PLAYBACK

#include <stdlib.h>

#include "commsRecorder.h"
#include "coretech/common/engine/utils/timer.h"
#include "anki/messaging/shared/utilMessaging.h"
#include <assert.h>

namespace Anki {
namespace Vector {

void CommsRecord::Print()
{
  printf("aTime %lld, timeStamp %lld, sourceVehicleId %d, packetSize %d ", time, timeStamp, sourceVehicleId, bufferSize);
  for (unsigned int i = 0; i < bufferSize; ++i)
    printf("%d[%x]", (int)buffer[i], (int)buffer[i]);
  printf("\n");
}


// Constructor
CommsRecorder::CommsRecorder(string & fileName, Comms::IComms *realComms)
: fileName_(fileName), realComms_(realComms)
{
}

// Destructor
CommsRecorder::~CommsRecorder()
{
  if(!outStream.fail())
    outStream.close();
}

//////////////////////////
// ICOMMS implementation


bool CommsRecorder::IsInitialized()
{
  return realComms_->IsInitialized();
}

u32 CommsRecorder::GetNumPendingMsgPackets()
{
  return realComms_->GetNumPendingMsgPackets();
}

size_t CommsRecorder::Send(const Comms::MsgPacket &msg)
{
  return realComms_->Send(msg);
}

bool CommsRecorder::IsConnected(s32 connectionId)
{
  return realComms_->IsConnected(connectionId);
}
  
/*
Recorded comms message layout:
 *** header ***
 -currentTime       (BaseStationTime_t / 8 bytes)
 -vehicleID         (int / 4 bytes)
 -messageTimeStamp  (BaseStationTime_t / 8 bytes)
 -messageSize       (unsigned char / 1 byte)
 *** payload ***
 -message           (messageSize bytes)
 */
  
#define COMMS_RECORD_HEADER_SIZE (sizeof(BaseStationTime_t) * 2 + sizeof(int) + sizeof(unsigned char))
bool CommsRecorder::GetNextMsgPacket(Comms::MsgPacket &msg)
{
  if (realComms_->GetNextMsgPacket(msg) && outStream.is_open())
  {
    // convert data to proper format and dump it to disk
    // prepare data
    BaseStationTime_t currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
    unsigned int headerSize = sizeof(BaseStationTime_t) + sizeof(msg.sourceId) + sizeof(BaseStationTime_t) + sizeof(msg.dataLen);
    unsigned char* header = new unsigned char [headerSize];
    
    unsigned int bytesPacked = 0;
    SafeUtilMsgPack(header, headerSize, &bytesPacked, "lilc", currentTime, msg.sourceId, msg.timeStamp, msg.dataLen);
    assert( bytesPacked == headerSize );

    outStream.write((const char*)header, bytesPacked);
    outStream.write((const char*)msg.data, msg.dataLen);
    outStream.flush();
    
    delete[] header;
    return true;
  }
  return false;
}

// Reads all data from buffer into provided data storage
BaseStationTime_t CommsRecorder::ReadFromBuffer(unsigned char *buffer, unsigned int size, vector<CommsRecord> &data)
{
  BaseStationTime_t timeLastMessageRecorded = 0;

  // process buffer
  unsigned char *bleBuffer;
  int sourceVehicleId = 0;
  unsigned char packetSize;
  BaseStationTime_t aTime = 0;

  unsigned int nextByte = 0;

  // make sure there is enough data on the buffer
  while (nextByte + COMMS_RECORD_HEADER_SIZE <= size)
  {
    BaseStationTime_t messageTimeStamp;
    unsigned int bytesRead = 0;
    SafeUtilMsgUnpack(buffer + nextByte, (size - nextByte), &bytesRead, "lilc", &aTime, &sourceVehicleId, &messageTimeStamp, &packetSize);
    assert( bytesRead == COMMS_RECORD_HEADER_SIZE );
    nextByte += bytesRead;
    
    if (nextByte + packetSize <= size)
    {
      bleBuffer = new unsigned char[packetSize];
      memcpy(bleBuffer, &buffer[nextByte], packetSize);
      CommsRecord datum;
      datum.buffer = bleBuffer;
      datum.bufferSize = packetSize;
      datum.sourceVehicleId = sourceVehicleId;
      datum.time = aTime;
      datum.timeStamp = messageTimeStamp;
      data.push_back(datum);
#ifdef DEBUG_RECORDING_PLAYBACK
      printf("READ %d ", (int)data.size());
      datum.Print();
#endif
      // message buffer
      nextByte += packetSize;

      if (timeLastMessageRecorded < aTime)
        timeLastMessageRecorded = aTime;
    }
  }

  //printf("packets read %d\n", dataSize);
  return timeLastMessageRecorded;
}
  
u32 CommsRecorder::GetNumMsgPacketsInSendQueue(s32 connectionId) {
  return realComms_->GetNumMsgPacketsInSendQueue(connectionId);
}
  
void CommsRecorder::Update(bool send_queued_msgs) {
  realComms_->Update(send_queued_msgs);
}
  
// ICOMMS implementation
//////////////////////////

// Called when it is appropriate to do large ammount of io
void CommsRecorder::PrepareLogFile()
{
  outStream.open(fileName_.c_str(), ofstream::binary);
  count_ = 0;
}


// prints out buffer array
void CommsRecorder::PrintfCharArray(unsigned char *buffer, unsigned int size)
{
  for (unsigned int i = 0; i < size; ++i)
    printf("%d[%x]", (int)buffer[i], (int)buffer[i]);
}

void CommsRecorder::ClearMsgPackets()
{
  // just pass it on to the real comms
  realComms_->ClearMsgPackets();
}


} // end namespace Vector
} // end namespace Anki

#endif
