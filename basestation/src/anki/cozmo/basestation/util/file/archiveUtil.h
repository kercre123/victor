/**
 * File: archiveUtil.h
 *
 * Author: Lee Crippen
 * Created: 4/4/2016
 *
 * Description:
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
  static void CreateArchiveFromFiles(const std::string& outputPath,
                                     const std::string& filenameBase,
                                     const std::vector<std::string>& filenames);
private:

};
  
} // end namespace Cozmo
} // end namespace Anki


#endif //__Basestation_Util_File_ArchiveUtil_H_

