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

#ifndef ANIMPROCESS_COZMO_ALEXA_MEDIA_ATTACHMENTREADER_H
#define ANIMPROCESS_COZMO_ALEXA_MEDIA_ATTACHMENTREADER_H

#include "reader.h"
#include <memory>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {
class AttachmentReader;
}
}
}
}

namespace Anki {
namespace Vector {

class AttachmentReader : public AlexaReader
{
public:

  explicit AttachmentReader(std::shared_ptr<alexaClientSDK::avsCommon::avs::attachment::AttachmentReader> reader);

  virtual size_t GetNumUnreadBytes() override;
  virtual size_t Read(uint8_t* buf, size_t toRead, Status& status) override;
  virtual void Close() override;

private:
  std::shared_ptr<alexaClientSDK::avsCommon::avs::attachment::AttachmentReader> _reader;
};

}
}

#endif
