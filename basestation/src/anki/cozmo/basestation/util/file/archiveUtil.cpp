/**
 * File: archiveUtil.cpp
 *
 * Author: Lee Crippen
 * Created: 4/4/2016
 *
 * Description: Utility wrapper around needed archive file creation functionality.
 *
 * Copyright: Anki, inc. 2016
 *
 */
#include "anki/cozmo/basestation/util/file/archiveUtil.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"


#if ANKI_DEV_CHEATS
#include "archive.h"
#include "archive_entry.h"

#include <sys/stat.h>
#include <fstream>
#endif

namespace Anki {
namespace Cozmo {

  
#if ANKI_DEV_CHEATS
bool ArchiveUtil::CreateArchiveFromFiles(const std::string& outputPath,
                                         const std::string& filenameBase,
                                         const std::vector<std::string>& filenames)
{
  struct archive* newArchive = archive_write_new();
  if (nullptr == newArchive)
  {
    PRINT_NAMED_ERROR("ArchiveUtil.CreateArchiveFromFiles", "Could not alloc new archive");
  }
  
  int errorCode = ARCHIVE_OK;
  errorCode = archive_write_add_filter_gzip(newArchive);
  if (ARCHIVE_OK != errorCode)
  {
    PRINT_NAMED_ERROR("ArchiveUtil.CreateArchiveFromFiles", "Error %s setting up archive", GetArchiveErrorString(errorCode));
    archive_write_free(newArchive);
    return false;
  }
  
  errorCode = archive_write_set_format_pax_restricted(newArchive);
  if (ARCHIVE_OK != errorCode)
  {
    PRINT_NAMED_ERROR("ArchiveUtil.CreateArchiveFromFiles", "Error %s setting up archive", GetArchiveErrorString(errorCode));
    archive_write_free(newArchive);
    return false;
  }
  
  errorCode = archive_write_open_filename(newArchive, outputPath.c_str());
  if (ARCHIVE_OK != errorCode)
  {
    PRINT_NAMED_ERROR("ArchiveUtil.CreateArchiveFromFiles", "Error %s opening file for archive", GetArchiveErrorString(errorCode));
    archive_write_close(newArchive);
    archive_write_free(newArchive);
    return false;
  }
  
  struct archive_entry* entry = archive_entry_new();
  if (nullptr == entry)
  {
    PRINT_NAMED_ERROR("ArchiveUtil.CreateArchiveFromFiles", "Could not alloc new entry");
    archive_write_close(newArchive);
    archive_write_free(newArchive);
    return false;
  }
  
  char buff[8192];
  
  for (auto& filename : filenames)
  {
    struct stat st;
    stat(filename.c_str(), &st);
    archive_entry_clear(entry);
    auto newFilename = RemoveFilenameBase(filenameBase, filename);
    archive_entry_set_pathname(entry, newFilename.c_str());
    archive_entry_set_size(entry, st.st_size);
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    
    errorCode = archive_write_header(newArchive, entry);
    if (ARCHIVE_OK != errorCode)
    {
      PRINT_NAMED_ERROR("ArchiveUtil.CreateArchiveFromFiles", "Error %s writing file header", GetArchiveErrorString(errorCode));
      archive_entry_free(entry);
      archive_write_close(newArchive);
      archive_write_free(newArchive);
      return false;
    }
    
    std::ifstream file(filename, std::ios::binary);
    file.read(buff, sizeof(buff));
    auto len = file.gcount();
    while ( len > 0 ) {
      archive_write_data(newArchive, buff, len);
      file.read(buff, sizeof(buff));
      len = file.gcount();
    }
  }
  
  archive_entry_free(entry);
  archive_write_close(newArchive);
  archive_write_free(newArchive);
  
  return true;
}
  
std::string ArchiveUtil::RemoveFilenameBase(const std::string& filenameBase, const std::string& filename)
{
  auto lastSep = filename.find_last_of('/');
  // We don't want to mess with a filename that has no path separators in it
  if (!filenameBase.empty() && lastSep != std::string::npos)
  {
    std::size_t lastMatchChar = std::string::npos;
    for (int i=0; i < filenameBase.length() && i <= lastSep; i++)
    {
      if (filenameBase[i] != filename[i])
      {
        break;
      }
      lastMatchChar = i;
    }
    
    if (lastMatchChar != std::string::npos)
    {
      return filename.substr(lastMatchChar+1);
    }
  }
  return filename;
}
  
const char* ArchiveUtil::GetArchiveErrorString(int errorCode)
{
  switch (errorCode)
  {
    case ARCHIVE_EOF: return "ARCHIVE_EOF";
    case ARCHIVE_OK: return "ARCHIVE_OK";
    case ARCHIVE_RETRY: return "ARCHIVE_RETRY";
    case ARCHIVE_WARN: return "ARCHIVE_WARN";
    case ARCHIVE_FAILED: return "ARCHIVE_FAILED";
    case ARCHIVE_FATAL: return "ARCHIVE_FATAL";
    default: return "UNKNOWN";
  }
}
  
#else // #if ANKI_DEV_CHEATS
  
bool ArchiveUtil::CreateArchiveFromFiles(const std::string& outputPath, const std::string& filenameBase, const std::vector<std::string>& filenames)
{
  return false;
}

std::string ArchiveUtil::RemoveFilenameBase(const std::string& filenameBase, const std::string& filename)
{
  return "";
}

const char* ArchiveUtil::GetArchiveErrorString(int errorCode)
{
  return nullptr;
}

#endif

} // end namespace Cozmo
} // end namespace Anki