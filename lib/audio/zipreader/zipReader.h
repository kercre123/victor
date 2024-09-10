//
//
//  ZipReader
//  Class to read data from a .zip file
//  Very limited:
//  1.  Only supports .zip files where the entries are stored uncompressed
//  2.  Assumes size of end of central directory (i.e. no comments)
//
//
//  Created by Stuart Eichert on 9/2/2015
//  Copyright (c) 2015 Anki. All rights reserved.
//

#ifndef _zip_reader_h_
#define _zip_reader_h_

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace Anki {
namespace AudioEngine {


class ZipReader {
  typedef struct {
    uint32_t offset;
    uint32_t length;
  } ZipEntry;

  typedef struct __attribute__ ((__packed__)) {
    uint32_t signature;
    uint16_t versionNeededToExtract;
    uint16_t generalPurposeBitFlag;
    uint16_t compressionMethod;
    uint16_t lastModFileTime;
    uint16_t lastModFileDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t fileNameLength;
    uint16_t extraFieldLength;
  } ZipLocalFileHeader;

  typedef struct __attribute__ ((__packed__)) {
    uint32_t signature;
    uint16_t versionMadeBy;
    uint16_t versionNeededToExtract;
    uint16_t generalPurposeBitFlag;
    uint16_t compressionMethod;
    uint16_t lastModFileTime;
    uint16_t lastModFileDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t fileNameLength;
    uint16_t extraFieldLength;
    uint16_t fileCommentLength;
    uint16_t diskNumberStart;
    uint16_t internalFileAttributes;
    uint32_t externalFileAttributes;
    uint32_t relativeOffsetOflocalHeader;
  } ZipGlobalFileHeader;

  typedef struct __attribute__ ((__packed__)) {
    uint32_t signature;
    uint16_t diskNumber;
    uint16_t centralDirectoryDiskNumber;
    uint16_t numEntriesThisDisk;
    uint16_t numEntries;
    uint32_t centralDirectorySize;
    uint32_t centralDirectoryOffset;
    uint16_t zipCommentLength;
  } ZipEndOfCentralDirectoryRecord;

public:
  ZipReader();
  ~ZipReader();

  bool Init(const std::string& pathToZipFile);

  static std::vector<std::string> SplitPath(const std::string& input);

  void Term();

  size_t Size();

  bool Find(const std::string& name, uint32_t& handle, uint32_t& size);

  bool Read(uint32_t handle, uint32_t offset, uint32_t size, void* dst);
  
  // Get Unsorted Entry name list
  const std::vector<std::string> GetEntityNames();

  static constexpr uint32_t kEndOfCentralDirectorySig = 0x06054B50;
  static constexpr uint32_t kGlobalFileHeaderSig = 0x02014B50;
  static constexpr uint32_t kLocalFileHeaderSig = 0x04034B50;

private:
  bool ReadEntries();
  bool ReadFromFile(void* dst, size_t offset, size_t n);

  int _fd;
  size_t _dataOffset;
  size_t _dataSize;
  std::unordered_map<std::string, ZipEntry> _entries;
};
  
} // Anki
} // AudioEngine

#endif // _zip_reader_h_
