#ifdef TARGET_ESPRESSIF
#include "c_types.h"
#endif
#include "transport/reliableSequenceId.h"

ReliableSequenceId PreviousSequenceId(ReliableSequenceId inSeqId)
{
  ReliableSequenceId retVal;
  if (inSeqId == k_MinReliableSeqId)
  {
    retVal = k_MaxReliableSeqId;
  }
  else
  {
    retVal = inSeqId - 1;
  }
  return retVal;
}
ReliableSequenceId NextSequenceId(ReliableSequenceId inSeqId)
{
  ReliableSequenceId retVal;
  if (inSeqId == k_MaxReliableSeqId)
  {
    retVal = k_MinReliableSeqId;
  }
  else
  {
    retVal = inSeqId + 1;
  }
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
