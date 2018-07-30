//
//  fileUtils.cpp
//  driveEngine
//
//  Created by Tony Xu on 3/2/15.
//
//

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeIostream.h"
#include "util/math/numericCast.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

namespace Anki {
namespace Util {

namespace {
# ifdef _WIN32
  const char kFileSeparator = '\\';
# else
  const char kFileSeparator = '/';
# endif
} // anonymous namespace
  
bool FileUtils::DirectoryExists(const std::string &path)
{
  struct stat info;
  bool exists = false;
  if( stat(path.c_str(), &info)==0 ) {
    if( info.st_mode & S_IFDIR ) {
      exists = true;
    }
  }
  return exists;
}

ssize_t FileUtils::GetFileSize(const std::string &path)
{
  struct stat info;
  if( stat(path.c_str(), &info)==0 ) {
    return numeric_cast<ssize_t>(info.st_size);
  }
  else {
    return -1;
  }
}

// -p      Create intermediate directories as required.  If this option is not specified,
// the full path prefix of each operand must already exist.  On the other hand, with this
// option specified, no error will be reported if a directory given as an operand already exists.
bool FileUtils::CreateDirectory(const std::string &path, const bool stripFilename, bool dashP)
{
  std::string workPath;
  if (stripFilename) {
    size_t pos = path.rfind('/');
    workPath = path.substr(0,pos);
  } else {
    workPath = path;
  }

  if (dashP) {
    size_t MAX_LOOP = 200;
    size_t pos = 0;
    size_t count = 0;
    while (pos < workPath.length() && count < MAX_LOOP) {
      pos = workPath.find('/', pos + 1);
      std::string subPath = workPath.substr(0, pos);
      ++count;
      if (!Util::FileUtils::DirectoryExists(subPath)) {
        if( mkdir(subPath.c_str(), S_IRWXU) != 0 ) {
          return false;
        }
      }
    }
    return count < MAX_LOOP;
  }

  if (!Util::FileUtils::DirectoryExists(workPath)) {
    if( mkdir(workPath.c_str(), S_IRWXU) != 0 ) {
      return false;
    }
  }
  return true;
}
  
void FileUtils::RemoveDirectory(const std::string &path)
{
  if( DirectoryExists(path) ) {
    DIR* dir = opendir(path.c_str());
    if( dir != NULL ) {
      struct dirent *fileInfo;
      while( (fileInfo = readdir(dir)) ) {
        if( fileInfo->d_type==DT_REG ) {
          std::string fileName(fileInfo->d_name);
          FileUtils::DeleteFile(path + "/" + fileName);
        }
        else if( fileInfo->d_type==DT_DIR ) {
          std::string subPath(fileInfo->d_name);
          if( subPath!="." && subPath!=".." ) {
            FileUtils::RemoveDirectory(path + "/" + subPath);
          }
        }
      }
      closedir(dir);
    }
    remove(path.c_str());
  }
}

ssize_t FileUtils::GetDirectorySize(const std::string& path)
{
  ssize_t size = 0;

  const auto files = FilesInDirectory(path, false, nullptr, true);
  for(const auto& file : files)
  {
    size += GetFileSize(file);
  }

  return size;
}

std::vector<std::string> FileUtils::FilesInDirectory(const std::string& path,
                                                     bool useFullPath,
                                                     const char* withExtension,
                                                     bool recurse)
{
  std::vector<const char*> withExtensions;
  if(nullptr != withExtension && withExtension[0]!=0) {
    withExtensions.push_back(withExtension);
  }
  return FileUtils::FilesInDirectory(path, useFullPath, withExtensions, recurse);
}
  
std::vector<std::string> FileUtils::FilesInDirectory(const std::string& path,
                                                     bool useFullPath,
                                                     const std::vector<const char*>& withExtensions,
                                                     bool recurse)
{
  // We always want to useFullPath when looking for files recursively
  useFullPath |= recurse;
  
  std::vector<std::string> files;
  
  std::function<bool(const char* fname)> FilterFilename = [](const char*) { return true; };
  
  if(!withExtensions.empty()) {
    FilterFilename = [&withExtensions](const char* fname) {
      for(auto* ext : withExtensions) {
        if(FilenameHasSuffix(fname, ext)) {
          return true;
        }
      }
      return false;
    };
  }
  
  DIR* listingDir = opendir(path.c_str());
  if( listingDir ) {
    struct dirent *info;
    while( (info = readdir(listingDir)) ) {
      if( info->d_type==DT_REG && FilterFilename(info->d_name) ) {
        std::string fileName(info->d_name);
        if(useFullPath) {
          files.push_back(FullFilePath({path, std::move(fileName)}));
        } else {
          files.push_back(std::move(fileName));
        }
      }
      else if(recurse &&
              info->d_type == DT_DIR &&
              strcmp(info->d_name, ".") != 0  &&
              strcmp(info->d_name, "..") != 0)
      {
        // Note we simply pass true as the recurse and useFullPath values here, since we already know we want to recurse
        auto recurseResults = FilesInDirectory(FullFilePath({path, info->d_name}), true, withExtensions, true);
        files.insert(files.end(), std::make_move_iterator(recurseResults.begin()), std::make_move_iterator(recurseResults.end()));
      }
    }
    closedir(listingDir);
  }
  
  return files;
}

bool FileUtils::TouchFile(const std::string& fileName, mode_t mode)
{
  int result = utime(fileName.c_str(), NULL);
  if (result == 0) {
    return true;
  }
  if (errno != ENOENT) {
    return false;
  }
  int fd = open(fileName.c_str(), O_RDWR | O_CREAT, mode);
  if (fd >= 0) {
    (void) close(fd);
    return true;
  }
  return false;
}

bool FileUtils::FileExists(const std::string& fileName)
{
  struct stat info;
  bool exists = false;
  if( stat(fileName.c_str(), &info)==0 ) {
    if( info.st_mode & S_IFREG ) {
      exists = true;
    }
  }
  return exists;
}

std::string FileUtils::ReadFile(const std::string& fileName)
{
  std::ifstream fileIn;
  fileIn.open(fileName, std::ios::in | std::ifstream::binary);
  if( fileIn.is_open() ) {
    std::string body((std::istreambuf_iterator<char>(fileIn)), std::istreambuf_iterator<char>());
    fileIn.close();
    return body;
  }
  else {
    std::string body;
    return body;
  }
}
  
std::vector<uint8_t> FileUtils::ReadFileAsBinary(const std::string& fileName)
{
  std::ifstream fileIn;
  fileIn.open(fileName, std::ios::in | std::ifstream::binary);
  if( fileIn.is_open() ) {
    fileIn.unsetf(std::ios::skipws);
    std::vector<uint8_t> data{std::istreambuf_iterator<char>(fileIn), std::istreambuf_iterator<char>()};
    fileIn.close();
    return data;
  }
  else {
    return {};
  }
}

bool FileUtils::WriteFile(const std::string &fileName, const std::string &body)
{
  std::vector<uint8_t> bytes;
  copy(body.begin(), body.end(), back_inserter(bytes));
  return WriteFile(fileName, bytes);
}
  
bool FileUtils::WriteFile(const std::string &fileName, const std::vector<uint8_t> &body, bool append)
{
  bool success = false;
  std::ofstream fileOut;
  std::ios_base::openmode mode = std::ios::out | std::ofstream::binary;
  if( append ) {
    mode |= std::ios::app;
  }
  fileOut.open(fileName,mode);
  if( fileOut.is_open() ) {
    copy(body.begin(), body.end(), std::ostreambuf_iterator<char>(fileOut));
    fileOut.flush();
    fileOut.close();
    success = true;
  }
  return success;
}

bool FileUtils::WriteFileAtomic(const std::string& fileName, const std::string& body)
{
  std::vector<uint8_t> bytes;
  copy(body.begin(), body.end(), back_inserter(bytes));
  return WriteFileAtomic(fileName, bytes);
}

bool FileUtils::WriteFileAtomic(const std::string& fileName, const std::vector<uint8_t>& body)
{
  std::string tmpFileName = fileName + ".tmp";
  DeleteFile(tmpFileName);
  bool success = WriteFile(tmpFileName, body) && (0 == rename(tmpFileName.c_str(), fileName.c_str()));
  if (!success) {
    DeleteFile(tmpFileName);
  }
  return success;
}

static bool CopyOrMoveFile(const std::string& dest,
                           const std::string& srcFileName,
                           const int maxBytesToCopyFromEnd,
                           const bool deleteSource)
{
  if (!FileUtils::FileExists(srcFileName) || dest.empty()) {
    return false;
  }

  // If dest is a file use that.
  // If dest is a folder use the src file name.
  std::string outFileName = dest;
  if (FileUtils::GetFileName(dest, true).empty()) {

    // dest is the output directory name
    // Get the output file name
    // Remove trailing separator if there is one
    while (outFileName.back() == kFileSeparator) {
      outFileName.pop_back();
    }

    outFileName += kFileSeparator + FileUtils::GetFileName(srcFileName, false);
  }

  // Create output directory in case it doesn't exist already
  FileUtils::CreateDirectory(outFileName, true, true);

  if (!maxBytesToCopyFromEnd && deleteSource) {
    // If we are moving the file on the same mounted filesystem it should succeed
    if (0 == rename(srcFileName.c_str(), outFileName.c_str())) {
      return true;
    }
    if (errno != EXDEV) {
      return false;
    }
    // A simple rename won't work.  The source and destination are on two different
    // mounted filesystems.  A copy is required.
  }

  std::ifstream inFile(srcFileName.c_str(), std::ios::binary);

  // Seek to appropriate starting position of input file
  if (maxBytesToCopyFromEnd != 0) {

    // Get file size
    inFile.seekg(0, std::ios_base::end);
    int fileSize = static_cast<int>(inFile.tellg());

    // Set stream position to given offset if file size > offset
    if (fileSize > maxBytesToCopyFromEnd) {
      inFile.seekg(-std::abs(maxBytesToCopyFromEnd), std::ios_base::end);
    } else {
      inFile.seekg(0, std::ios_base::beg);
    }
  }

  // Copy file
  std::ofstream outFile(outFileName.c_str(), std::ios::binary);
  outFile << inFile.rdbuf();
  outFile.close();
  inFile.close();

  if (deleteSource) {
    FileUtils::DeleteFile(srcFileName);
  }

  return true;
}

bool FileUtils::CopyFile(const std::string& dest, const std::string& srcFileName, const int maxBytesToCopyFromEnd)
{
  return CopyOrMoveFile(dest, srcFileName, maxBytesToCopyFromEnd, false /* deleteSource */);
}

bool FileUtils::MoveFile(const std::string& dest, const std::string& srcFileName)
{
  return CopyOrMoveFile(dest, srcFileName, 0, true /* deleteSource */);
}

void FileUtils::DeleteFile(const std::string &fileName)
{
  (void) remove(fileName.c_str());
}
 
void FileUtils::ListAllDirectories( const std::string& path, std::vector<std::string>& directories )
{
  if ( !directories.empty() )
  {
    directories.clear();
  }
  
  if( FileUtils::DirectoryExists(path) )
  {
    DIR* dir = opendir(path.c_str());
    if( dir != NULL )
    {
      struct dirent *fileInfo;
      while( (fileInfo = readdir(dir)) )
      {
        if( fileInfo->d_type==DT_DIR )
        {
          std::string directory(fileInfo->d_name);
          if( directory!="." && directory!=".." )
          {
            directories.emplace_back( std::move( directory ) );
          }
        }
      }
      closedir(dir);
    }
  }
}

bool FileUtils::FilenameHasSuffix(const char* inFilename, const char* inSuffix)
{
  const size_t filenameLen = strlen(inFilename);
  const size_t suffixLen   = strlen(inSuffix);
  
  if (filenameLen < suffixLen)
  {
    return false;
  }
  
  const int cmp = strcmp(&inFilename[filenameLen-suffixLen], inSuffix);
  return (cmp == 0);
}

  
namespace {
  inline void RemoveFileSeparators(std::string& str, bool removeLeading = true)
  {
    if(!str.empty())
    {
      const auto strLen = str.length();
      
      const bool hasLeading = removeLeading && (str[0] == kFileSeparator);
      const bool hasTrailing = (str.back() == kFileSeparator);
      
      const auto start = (hasLeading  ? 1        : 0);
      const auto end   = (hasTrailing ? strLen-1 : strLen);
      
      str = str.substr(start, end-start);
    }
  }
} // anonymous namespace
  
std::string FileUtils::FullFilePath(std::vector<std::string>&& names)
{
  std::string fullpath;

  if(!names.empty())
  {
    auto nameIter = names.begin();
    
    fullpath = *nameIter;

    // NOTE: Leave leading fileSep for first entry only
    RemoveFileSeparators(fullpath, false);
    
    ++nameIter;
    while(nameIter != names.end())
    {
      if(!nameIter->empty())
      {
        RemoveFileSeparators(*nameIter);
        
        if(!fullpath.empty()) {
          fullpath += kFileSeparator;
        }
        fullpath += *nameIter;
      }
      ++nameIter;
    }
  }

  return fullpath;
}

std::string FileUtils::GetFileName(const std::string& fullPath, bool mustHaveExtension, bool removeExtension)
{
  size_t i = fullPath.rfind(kFileSeparator, fullPath.length());
  if (i != std::string::npos &&  i != fullPath.length() - 1) {
    std::string potentialFile = fullPath.substr(i+1, fullPath.length() - i);
    size_t j = potentialFile.find(".");
    if (!mustHaveExtension || j != std::string::npos) {
      if(removeExtension) {
        return potentialFile.substr(0, j);
      }
      return potentialFile;
    }
  }
  
  return("");
}

std::string FileUtils::AddTrailingFileSeparator(const std::string& path)
{
  std::string newPath;
  newPath = path;
  if (newPath.back() != kFileSeparator) {
    newPath += kFileSeparator;
  }
  return newPath;
}
  
}
}
