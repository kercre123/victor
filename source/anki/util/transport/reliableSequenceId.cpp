/**
 * File: reliableSequenceId
 *
 * Author: Mark Wesley
 * Created: 02/04/15
 *
 * Description: Sequential looping id for use in reliable message transport
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/transport/reliableSequenceId.h"
#include <assert.h>


namespace Anki {
namespace Util {


ReliableSequenceId PreviousSequenceId(ReliableSequenceId inSeqId)
{
  assert ((inSeqId >= k_MinReliableSeqId) && (inSeqId <= k_MaxReliableSeqId));
  
  ReliableSequenceId retVal;
  if (inSeqId == k_MinReliableSeqId)
  {
    retVal = k_MaxReliableSeqId;
  }
  else
  {
    assert(inSeqId > k_MinReliableSeqId);
    retVal = inSeqId - 1;
  }
  
  assert ((inSeqId >= k_MinReliableSeqId) && (inSeqId <= k_MaxReliableSeqId));
  return retVal;
}


ReliableSequenceId NextSequenceId(ReliableSequenceId inSeqId)
{
  assert ((inSeqId >= k_MinReliableSeqId) && (inSeqId <= k_MaxReliableSeqId));
  
  ReliableSequenceId retVal;
  if (inSeqId == k_MaxReliableSeqId)
  {
    retVal = k_MinReliableSeqId;
  }
  else
  {
    assert(inSeqId < k_MaxReliableSeqId);
    retVal = inSeqId + 1;
  }
  
  assert ((inSeqId >= k_MinReliableSeqId) && (inSeqId <= k_MaxReliableSeqId));
  return retVal;
}


bool IsSequenceIdInRange(ReliableSequenceId seqId, ReliableSequenceId minSeqId, ReliableSequenceId maxSeqId)
{
  if (maxSeqId >= minSeqId)
  {
    // not looped
    return ((seqId >= minSeqId) && (seqId <= maxSeqId));
  }
  else
  {
    // Ids have looped around
    return ((seqId >= minSeqId) || (seqId <= maxSeqId));
  }
}


} // end namespace Util
} // end namespace Anki
