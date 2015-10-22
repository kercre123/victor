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

#ifndef __NetworkService_ReliableSequenceId_H__
#define __NetworkService_ReliableSequenceId_H__

#ifdef __cplusplus
namespace Anki {
namespace Util {
extern "C" {
#endif

typedef uint16_t  ReliableSequenceId;

static const ReliableSequenceId k_InvalidReliableSeqId = 0;
static const ReliableSequenceId k_MinReliableSeqId = 1;
static const ReliableSequenceId k_MaxReliableSeqId = 65534;


ReliableSequenceId PreviousSequenceId(ReliableSequenceId inSeqId);
ReliableSequenceId NextSequenceId(ReliableSequenceId inSeqId);
bool IsSequenceIdInRange(ReliableSequenceId seqId, ReliableSequenceId minSeqId, ReliableSequenceId maxSeqId);

#ifdef __cplusplus
} // end extern "C"
} // end namespace Util
} // end namespace Anki
#endif

#endif // __NetworkService_ReliableSequenceId_H__
