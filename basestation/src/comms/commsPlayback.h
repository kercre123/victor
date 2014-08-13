/**
 * File: commsLogReader.h
 *
 * Author: Damjan Stulic
 * Created: 7/22/2012
 *
 * Description: Reads received messages from a file, and passes them on
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#ifndef BASESTATION_COMMS_COMMSLOGREADER_H_
#define BASESTATION_COMMS_COMMSLOGREADER_H_

#include "comms/commsRecorder.h"
#include "anki/common/basestation/utils/helpers/includeFstream.h"
#include "anki/common/basestation/utils/helpers/includeIostream.h"
#include <stdio.h>
#include <string>
#include <vector>

using namespace std;


namespace Anki {
namespace Cozmo {

/**
* Reads received messages from a file, and passes them on
*/
class CommsPlayback : public CommsRecorder {
public:

  // Constructor
  CommsPlayback(string & fileName, Comms::IComms *realComms);

  // Destructor
  virtual ~CommsPlayback();

  // Called when it is appropriate to do large amount of io
  virtual void PrepareLogFile();

  // returns the highest timestamp found in log file
  BaseStationTime_t GetTimeLastMessageRecorded() { return timeLastMessageRecorded; };

  //////////////////////////
  // ICOMMS implementation
  virtual bool IsInitialized();
  virtual u32 GetNumPendingMsgPackets();
  virtual bool GetNextMsgPacket(Comms::MsgPacket &msg);
  virtual size_t Send(const Comms::MsgPacket &msg);
  // prevents parent class method from executing
  //virtual void SetCurrentTimestamp(BaseStationTime_t timestamp) { };
  // prevents parent class method from executing
  virtual void ClearMsgPackets() { };
  // ICOMMS implementation
  //////////////////////////

protected:

  // Reads all data from file into internal data storage
  virtual void ReadFromFile();

  // internal storage
  unsigned int nextValue;
  vector<CommsRecord> dataFromFile;

  BaseStationTime_t timeLastMessageRecorded;
};


} // end namespace Cozmo
} // end namespace Anki

#endif //BASESTATION_COMMS_COMMSLOGREADER_H_
