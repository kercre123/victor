/**
 * File: commsLogWriter.h
 *
 * Author: Damjan Stulic
 * Created: 7/22/2012
 *
 * Description: Writes received messages to a file, and passes them on
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#if COZMO_RECORDING_PLAYBACK

#ifndef BASESTATION_COMMS_COMMSLOGWRITER_H_
#define BASESTATION_COMMS_COMMSLOGWRITER_H_

#include "anki/messaging/basestation/IComms.h"
#include "coretech/common/engine/utils/helpers/includeFstream.h"
#include "coretech/common/engine/utils/helpers/includeIostream.h"
#include <stdio.h>
#include <string>
#include <vector>
using namespace std;


namespace Anki {
namespace Vector {

/**
* Defines a storage for values read from file
*/
struct CommsRecord
{
  unsigned long long int time;
  int sourceVehicleId;
  unsigned char *buffer;
  unsigned char bufferSize;
  BaseStationTime_t timeStamp;
  void Print();
};


/**
* Writes received messages to a file, and passes them on
*/
class CommsRecorder : public Comms::IComms{
public:
  // Constructor
  CommsRecorder(string & fileName, Comms::IComms *realComms);

  // Destructor
  virtual ~CommsRecorder();

  // Called when it is appropriate to do large ammount of io
  virtual void PrepareLogFile();

  //////////////////////////
  // ICOMMS implementation
  virtual bool IsInitialized();
  virtual u32 GetNumPendingMsgPackets();
  virtual bool IsConnected(s32 connectionId);
  virtual size_t Send(const Comms::MsgPacket &msg);
  virtual bool GetNextMsgPacket(Comms::MsgPacket &msg);
  //virtual void SetCurrentTimestamp(BaseStationTime_t timestamp) { realComms_->SetCurrentTimestamp(timestamp); }
  // Reads all data from buffer into provided data storage
  BaseStationTime_t ReadFromBuffer(unsigned char *buffer, unsigned int size, vector<CommsRecord> &data);
  virtual void ClearMsgPackets();
  virtual u32 GetNumMsgPacketsInSendQueue(s32 connectionId);
  virtual void Update(bool send_queued_msgs = true);
  // ICOMMS implementation
  //////////////////////////

protected:

  // prints out buffer array
  virtual void PrintfCharArray(unsigned char *buffer, unsigned int size);

  // internal storage

  // file name for recording data
  string fileName_;

  // real communication layer
  IComms *realComms_;

  // stream for dumping recorded data out to a file
  ofstream outStream;

  unsigned int count_;

};


} // end namespace Vector
} // end namespace Anki

#endif //BASESTATION_COMMS_COMMSLOGWRITER_H_

#endif
