/**
 * File: reader.h
 *
 * Author: baustin
 * Created: Oct 30 2018
 *
 * Description: Abstraction for reading data from an underlying stream
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#ifndef ANIMPROCESS_COZMO_ALEXA_MEDIA_READER_H
#define ANIMPROCESS_COZMO_ALEXA_MEDIA_READER_H

#include <cstddef>
#include <cstdint>

namespace Anki {
namespace Vector {

class AlexaReader
{
public:

  virtual ~AlexaReader() {}

  enum class Status : uint8_t {
    Ok,
    WouldBlock,
    Timeout,
    Error
  };

  static const char* StatusToString(const Status& status) {
    switch(status) {
      case Status::Ok: return "Ok";
      case Status::WouldBlock: return "WouldBlock";
      case Status::Timeout: return "Timeout";
      case Status::Error: return "Error";
    }
  }

  virtual size_t GetNumUnreadBytes() = 0;
  virtual size_t Read(uint8_t* buf, size_t toRead, Status& status) = 0;
  virtual void Close() = 0;

};

}
}

#endif
