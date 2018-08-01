//
//  fileUtils.h
//  driveEngine
//
//  Created by Tony Xu on 3/2/15.
//
//

#ifndef __Util__FileUtils_H__
#define __Util__FileUtils_H__

#include <cstdint>
#include <stdio.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

namespace Anki {
namespace Util {

class FileUtils{
public:
  static bool DirectoryExists(const std::string& path);

  static bool DirectoryDoesNotExist(const std::string& path) { return !DirectoryExists(path); }

  // stripFilename - if set to true, will remove text after last '/'
  // dashP (-p)    - Create intermediate directories as required.  If this option is not specified,
  // the full path prefix of each operand must already exist.  On the other hand, with this
  // option specified, no error will be reported if a directory given as an operand already exists.
  static bool CreateDirectory(const std::string& path, const bool stripFilename = false, bool dashP = true);
  
  static void RemoveDirectory(const std::string& path);

  static ssize_t GetDirectorySize(const std::string& path);
  
  // If useFullPath is true, the returned vector will contain full paths to each file found
  // (i.e., path/to/file/filename.ext). If extensions are provided (with or without "."), only
  // files with any of those extensions (case sensitive) will be returned. Use nullptr or "" for
  // withExtension to accept any extension. Enabling recure will also enable useFullPath.
  static std::vector<std::string> FilesInDirectory(const std::string& path, bool useFullPath = false,
                                                   const char* withExtension = nullptr, bool recurse = false);
  static std::vector<std::string> FilesInDirectory(const std::string& path, bool useFullPath,
                                                   const std::vector<const char*>& withExtensions, bool recurse = false);

  static bool TouchFile(const std::string& fileName,
                        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  static bool FileExists(const std::string& fileName);

  static ssize_t GetFileSize(const std::string& fileName);

  static bool FileDoesNotExist(const std::string& fileName) { return !FileExists(fileName); }

  static std::string ReadFile(const std::string& fileName);

  static std::vector<uint8_t> ReadFileAsBinary(const std::string& fileName);

  static bool WriteFile(const std::string& fileName, const std::string& body);
  
  static bool WriteFile(const std::string& fileName, const std::vector<uint8_t>& body, bool append = false);
  
  static bool WriteFileAtomic(const std::string& fileName, const std::string& body);

  static bool WriteFileAtomic(const std::string& fileName, const std::vector<uint8_t>& body);

  // Copies srcFileName to dest.
  // If dest is a file, the srcFileName is copied to a file called dest.
  // If dest is a folder, the copy retains the name of the original file and is put in dest.
  // If maxBytesToCopyFromEnd != 0, it indicates the maximum number of bytes to be copied from the end
  // of srcFileName to dest. This is required of the playpen test app for copying the engine log but only
  // the part we care about for the last robot.
  static bool CopyFile(const std::string& dest, const std::string& srcFileName, const int maxBytesToCopyFromEnd = 0);

  // Moves srcFileName to dest
  // If dest is a file, the srcFileName is moved to a file called dest
  // If dest is a folder, the move retains the name of the original file and is put in dest.
  // Returns true if the move was successful, false otherwise
  static bool MoveFile(const std::string& dest, const std::string& srcFileName);

  static void DeleteFile(const std::string& fileName);
  
  static void ListAllDirectories( const std::string& path, std::vector<std::string>& directories );
  
  static bool FilenameHasSuffix(const char* inFilename, const char* inSuffix);
  
  // Insert a platform-specific file separater between the strings in the given vector.
  // Example FullFilePath({"path", "to", "file.txt"}) will return "path\\to\\file.txt" on
  // Windows and "path/to/file.txt" on other platforms. More complicated cases are
  // also handled, where the passed-in strings may or may not already have leading/trailing
  // file separators, to avoid duplicates.
  static std::string FullFilePath(std::vector<std::string>&& names);

  // Returns the file name from the full path.
  // If mustHaveExtension == true, the file name must contain an extension (i.e. a ".").
  // If "." is not present or the delimeter character is the last character of fullPath, returns "".
  // If mustHaveExtension == false, returns whatever follows the last delimiter in fullPath.
  // If mustHaveExtension and removeExtension == true, returns the filename with the extension removed
  static std::string GetFileName(const std::string& fullPath, bool mustHaveExtension = false, bool removeExtension = false);

  static std::string AddTrailingFileSeparator(const std::string& path);
  
};

} // namespace Util
} // namespace Anki

#endif // __Util__FileUtils_H__
