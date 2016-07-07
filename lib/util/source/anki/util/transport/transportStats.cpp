/**
 * File: transportStats
 *
 * Author: Mark Wesley
 * Created: 2/06/15
 *
 * Description: Stats for messages sent and received on a given transport connection or layer
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "util/transport/transportStats.h"
#include "util/logging/logging.h"
#include <assert.h>
#include <stdio.h>
#include <limits>


namespace Anki {
namespace Util {

  
const char* MessageErrorToString(EMessageError messageError)
{
  if ((messageError >= 0) && (messageError < eME_Count))
  {
  #define ANKI_MESSAGE_ERROR(expr)  # expr,
    static const char* k_Names[eME_Count] = { ANKI_MESSAGE_ERRORS };
  #undef ANKI_MESSAGE_ERROR
    return k_Names[messageError];
  }
  
  assert(0);
  return "ERROR";
}

  
// ============================== MessageStats ==============================
  

MessageStats::MessageStats(const char* inName)
: _name(inName)
{
  Reset();
}


void MessageStats::Reset()
{
  _numBytes         = 0;
  _numMessages      = 0;
  _smallestMessage  = std::numeric_limits<decltype(_smallestMessage)>::max();
  _largestMessage   = 0;
  
  for (int i=0; i < eME_Count; ++i)
  {
    _errorCounts[i] = 0;
  }
}
  

void MessageStats::AddMessage(uint32_t messageSize)
{
  if (messageSize < _smallestMessage)
  {
    _smallestMessage = messageSize;
  }
  if (messageSize > _largestMessage)
  {
    _largestMessage = messageSize;
  }
  
  _numBytes += messageSize;
  ++_numMessages;
}


void MessageStats::AddError(EMessageError messageError)
{
  if ((messageError >= 0) && (messageError < eME_Count))
  {
    _errorCounts[messageError]++;
  }
  else
  {
    assert(0);
  }
}
  
  
uint32_t MessageStats::GetErrorCount(EMessageError messageError) const
{
  if ((messageError >= 0) && (messageError < eME_Count))
  {
    return _errorCounts[messageError];
  }
  else
  {
    assert(0);
    return 0;
  }
}


void MessageStats::Print(const char* inName) const
{
  if (_numMessages > 0)
  {
    double averageBytes = (double)_numBytes / (double)_numMessages;
    PRINT_CH_INFO("Network", "MessageStats", "[%s][%s] %u messages, %llu bytes, avg = %2.2f bytes (%u..%u)", inName, _name, _numMessages, _numBytes, averageBytes, _smallestMessage, _largestMessage);
    for (int i=0; i < eME_Count; ++i)
    {
      if (_errorCounts[i] > 0)
      {
        PRINT_CH_INFO("Network", "MessageStats", "  ErrorCount %s = %u", MessageErrorToString(EMessageError(i)), _errorCounts[i] );
      }
    }
  }
}
  
  
// ============================== TransportStats ==============================
  

TransportStats::TransportStats(const char* inName)
  : _sentStats("Sent")
  , _recvStats("Recv")
  , _name(inName)
{
}
  

void TransportStats::Reset()
{
  _sentStats.Reset();
  _recvStats.Reset();
}


void TransportStats::AddRecvMessage(uint32_t messageSize)
{
  _recvStats.AddMessage(messageSize);
}
 
  
void TransportStats::AddSentMessage(uint32_t messageSize)
{
  _sentStats.AddMessage(messageSize);
}

  
void TransportStats::AddRecvError(EMessageError messageError)
{
  _recvStats.AddError(messageError);
}
    

void TransportStats::AddSendError(EMessageError messageError)
{
  _sentStats.AddError(messageError);
}


void TransportStats::Print() const
{
  _sentStats.Print(_name);
  _recvStats.Print(_name);
}


} // end namespace Util
} // end namespace Anki
