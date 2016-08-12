/**
 * File: archiveUtil.h
 *
 * Author: Lee Crippen
 * Created: 4/4/2016
 *
 * Description: Utility wrapper around needed archive file creation functionality.
 *
 * Copyright: Anki, inc. 2016
 *
 */
#ifndef __Basestation_Util_File_ArchiveUtil_H_
#define __Basestation_Util_File_ArchiveUtil_H_

#include <vector>
#include <string>

// Forward declarations

namespace Anki {
namespace Cozmo {
  
class ArchiveUtil {
public:
// mostly static methods for creating archive files
  static bool CreateArchiveFromFiles(const std::string& outputPath,
                                     const std::string& filenameBase,
                                     const std::vector<std::string>& filenames);
  
  // Removes an initial part of a filename (does nothing if the filename has no path separators)
  static std::string RemoveFilenameBase(const std::string& filenameBase, const std::string& filename);
  
private:
  static const char* GetArchiveErrorString(int errorCode);

};
  
} // end namespace Cozmo
} // end namespace Anki


#endif //__Basestation_Util_File_ArchiveUtil_H_

