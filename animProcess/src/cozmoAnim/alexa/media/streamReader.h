/**
 * File: streamReader.h
 *
 * Author: baustin
 * Created: Oct 30 2018
 *
 * Description: Reader for std::istream
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#ifndef ANIMPROCESS_COZMO_ALEXA_MEDIA_STREAMREADER_H
#define ANIMPROCESS_COZMO_ALEXA_MEDIA_STREAMREADER_H

#include "reader.h"
#include <iostream>
#include <memory>

namespace Anki {
namespace Vector {

class StreamReader : public AlexaReader
{
public:

  StreamReader(std::shared_ptr<std::istream> stream, bool repeat);

  virtual size_t GetNumUnreadBytes() override;
  virtual size_t Read(uint8_t* buf, size_t toRead, Status& status) override;
  virtual void Close() override;

private:
  void Restart();

  std::shared_ptr<std::istream> _stream;
  bool _repeat;
};

}
}

#endif
