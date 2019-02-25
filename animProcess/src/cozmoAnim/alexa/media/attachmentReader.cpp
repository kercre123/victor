/**
 * File: attachmentReader.h
 *
 * Author: baustin
 * Created: Oct 30 2018
 *
 * Description: Reader for AVS SDK's AttachmentReader class
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#include "attachmentReader.h"

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/AVS/Attachment/AttachmentReader.h>

namespace Anki {
namespace Vector {

using AVSReadStatus = alexaClientSDK::avsCommon::avs::attachment::AttachmentReader::ReadStatus;

AttachmentReader::AttachmentReader(
  std::shared_ptr<alexaClientSDK::avsCommon::avs::attachment::AttachmentReader> reader)
: _reader(std::move(reader))
{
}

size_t AttachmentReader::GetNumUnreadBytes()
{
  return (size_t)_reader->getNumUnreadBytes();
}

size_t AttachmentReader::Read(uint8_t* buf, size_t toRead, Status& status)
{
  AVSReadStatus avsStatus;
  auto ret = _reader->read(buf, toRead, &avsStatus);
  switch (avsStatus) {
    case AVSReadStatus::OK:
    case AVSReadStatus::OK_OVERRUN_RESET:
      status = Status::Ok;
      break;
    case AVSReadStatus::OK_WOULDBLOCK:
      status = Status::WouldBlock;
      break;
    case AVSReadStatus::OK_TIMEDOUT:
      status = Status::Timeout;
      break;
    default:
      status = Status::Error;
      break;
  }
  return ret;
}

void AttachmentReader::Close()
{
  _reader->close();
}

}
}
