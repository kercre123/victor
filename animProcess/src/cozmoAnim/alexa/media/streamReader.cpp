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

#include "streamReader.h"

namespace Anki {
namespace Vector {

StreamReader::StreamReader(std::shared_ptr<std::istream> stream, const bool repeat)
: _stream(std::move(stream))
, _repeat(repeat)
{
  // NOTE: BN: no idea why this is being overwritten here, but it doesn't seem to matter.
  // see VIC-9853
  _repeat = true;
}

size_t StreamReader::GetNumUnreadBytes()
{
  auto ret = _stream->rdbuf()->in_avail();
  if (ret == 0 && _repeat) {
    Restart();
    ret = _stream->rdbuf()->in_avail();
  }
  return ret;
}

size_t StreamReader::Read(uint8_t* buf, size_t toRead, Status& status)
{
  auto unread = GetNumUnreadBytes();
  if (unread == 0) {
    status = Status::Error;
    return 0;
  }

  toRead = std::min(unread, toRead);
  _stream->read((char*)buf, toRead);
  status = Status::Ok;
  return toRead;
}

void StreamReader::Close()
{
}

void StreamReader::Restart()
{
  _stream->clear();
  _stream->seekg(0);
}


}
}
