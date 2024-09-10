//
//
//  ZipReader
//  Class to read data from a .zip file
//
//  Created by Stuart Eichert on 9/2/2015
//  Copyright (c) 2015 Anki. All rights reserved.
//

#include "zipReader.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <sstream>

namespace Anki {
namespace AudioEngine {

  
ZipReader::ZipReader()
: _fd(-1)
, _dataOffset(0)
, _dataSize(0)
{
}

ZipReader::~ZipReader()
{
  Term();
}

bool ZipReader::Init(const std::string& pathToZipFile)
{
  Term();
  std::vector<std::string> parts = SplitPath(pathToZipFile);
  std::string zipFileToOpen = parts[0];
  struct stat info;
  memset(&info, 0, sizeof(info));
  if (!stat(zipFileToOpen.c_str(), &info)) {
    int fd = open(zipFileToOpen.c_str(), O_RDONLY);
    if (fd >= 0) {
      _fd = fd;
      _dataOffset = 0;
      _dataSize = (size_t) info.st_size;
      bool result = ReadEntries();
      if (!result) {
        Term();
      }
      if (parts.size() > 1) {
        uint32_t handle;
        uint32_t size;
        result = Find(parts[1], handle, size);
        if (result) {
          _dataOffset += handle;
          _dataSize = (size_t) size;
          _entries.clear();
          result = ReadEntries();
        }
        if (!result) {
          Term();
        }
      }
      return result;
    }
  }
  return false;
}

std::vector<std::string> ZipReader::SplitPath(const std::string& input)
{
  // Split the input at occurrences of '?' to indicate a file within a file
  std::vector<std::string> strings;
  std::istringstream iss(input);
  std::string item;
  while (std::getline(iss, item, '?')) {
    strings.push_back(item);
  }
  return strings;
}

bool ZipReader::ReadFromFile(void* dst, size_t offset, size_t n)
{
  if (_fd < 0) {
    return false;
  }
  off_t off = (off_t) (_dataOffset + offset);
  size_t nleft;
  ssize_t nread;
  char* ptr;

  ptr = (char *) dst;
  nleft = n;
  while (nleft > 0) {
    if ( (nread = pread(_fd, ptr, nleft, off)) < 0) {
      if (errno == EINTR) {
        nread = 0;
      } else {
        return -1;
      }
    } else if (nread == 0) {
      break;
    }
    nleft -= nread;
    ptr += nread;
    off += nread;
  }
  return (nleft == 0);
}

bool ZipReader::ReadEntries()
{
  bool rc;
  if (!_dataSize || _fd < 0) {
    return false;
  }

  // Search for the End of Central Directory at the end of the .zip file
  // Ideally it will be the last thing in the .zip file, but there could be additional
  // padding on the file.  If at first we don't succeed, rewind one byte at a time and keep
  // looking, up to maxExtraPadding bytes.

  ZipEndOfCentralDirectoryRecord endRecord;
  if (_dataSize < sizeof(endRecord)) {
    return false;
  }
  memset(&endRecord, 0, sizeof(endRecord));
  size_t extraPadding = 0;
  size_t maxExtraPadding = std::min((size_t) 1024, (size_t) (_dataSize - sizeof(endRecord)));

  do {
    rc = ReadFromFile(&endRecord, _dataSize - sizeof(endRecord) - extraPadding, sizeof(endRecord));
    if (!rc) {
      return false;
    }
  } while (endRecord.signature != kEndOfCentralDirectorySig && extraPadding++ < maxExtraPadding);

  if (endRecord.signature != kEndOfCentralDirectorySig) {
    return false;
  }

  size_t offset = endRecord.centralDirectoryOffset;
  for (uint16_t i = 0 ; i < endRecord.numEntries; i++) {
    ZipGlobalFileHeader globalFileHeader;
    rc = ReadFromFile(&globalFileHeader, offset, sizeof(globalFileHeader));
    if (!rc) {
      return false;
    }
    if (globalFileHeader.signature != kGlobalFileHeaderSig) {
      return false;
    }
    size_t fileNameOffset = offset + sizeof(globalFileHeader);
    if (globalFileHeader.uncompressedSize > 0) {
      std::vector<char> fileNameVec(globalFileHeader.fileNameLength, 0);
      rc = ReadFromFile(fileNameVec.data(), fileNameOffset, fileNameVec.size());
      if (!rc) {
        return false;
      }
      std::string fileName(fileNameVec.begin(), fileNameVec.end());
      ZipLocalFileHeader localFileHeader;
      rc = ReadFromFile(&localFileHeader, globalFileHeader.relativeOffsetOflocalHeader,
                        sizeof(localFileHeader));
      if (!rc) {
        return false;
      }
      if (localFileHeader.signature != kLocalFileHeaderSig) {
        return false;
      }
      uint32_t entryDataOffset = globalFileHeader.relativeOffsetOflocalHeader
        + sizeof(localFileHeader) + localFileHeader.fileNameLength
        + localFileHeader.extraFieldLength;

      ZipEntry entry = {entryDataOffset, globalFileHeader.uncompressedSize};
      _entries[fileName] = entry;
    }

    offset = fileNameOffset + globalFileHeader.fileNameLength
      + globalFileHeader.extraFieldLength + globalFileHeader.fileCommentLength;

  }
  return true;
}

void ZipReader::Term()
{
  _dataSize = 0;
  _dataOffset = 0;
  if (_fd >= 0) {
    (void) close(_fd); _fd = -1;
  }
  _entries.clear();
}

size_t ZipReader::Size()
{
  return _entries.size();
}

bool ZipReader::Find(const std::string& name, uint32_t& handle, uint32_t& size)
{
  handle = 0;
  size = 0;
  auto search = _entries.find(name);
  if (search != _entries.end()) {
    ZipEntry entry = search->second;
    handle = entry.offset;
    size = entry.length;
    return true;
  }
  return false;
}

bool ZipReader::Read(uint32_t handle, uint32_t offset, uint32_t size, void* dst)
{
  uint32_t end = handle + offset + size;
  if (end > _dataSize) {
    return false;
  }
  bool rc = ReadFromFile(dst, handle + offset, size);
  return rc;
}
  
const std::vector<std::string> ZipReader::GetEntityNames()
{
  std::vector<std::string> names;
  names.reserve(_entries.size());
  for (auto& kvp : _entries) {
    names.push_back(kvp.first);
  }
  
  return names;
}
  
} // Anki
} // AudioEngine
