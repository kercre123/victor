#include "anki/cozmo/basestation/util/file/archiveUtil.h"

#include "archive.h"
#include "archive_entry.h"

#include <sys/stat.h>
#include <fstream>

namespace Anki {
namespace Cozmo {

void ArchiveUtil::CreateArchiveFromFiles(const std::string& outputPath,
                                         const std::string& filenameBase,
                                         const std::vector<std::string>& filenames)
{
  struct archive* newArchive = archive_write_new();
  archive_write_add_filter_gzip(newArchive);
  archive_write_set_format_pax_restricted(newArchive);
  archive_write_open_filename(newArchive, outputPath.c_str());
  
  struct archive_entry* entry = archive_entry_new();
  char buff[8192];
  
  for (auto& filename : filenames)
  {
    struct stat st;
    stat(filename.c_str(), &st);
    archive_entry_clear(entry);
    std::string newFilename = filename;
    // If we were given a file name base, go through and remove that from the files listed
    if (!filenameBase.empty())
    {
      std::size_t lastMatchChar = std::string::npos;
      auto lastSep = filename.find_last_of('/');
      for (int i=0; i < filenameBase.length() && i <= lastSep && i != std::string::npos; i++)
      {
        if (filenameBase[i] != filename[i])
        {
          break;
        }
        lastMatchChar = i;
      }
      
      if (lastMatchChar != std::string::npos)
      {
        newFilename = newFilename.substr(lastMatchChar+1);
      }
    }
    archive_entry_set_pathname(entry, newFilename.c_str());
    archive_entry_set_size(entry, st.st_size);
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_write_header(newArchive, entry);
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
}

} // end namespace Cozmo
} // end namespace Anki