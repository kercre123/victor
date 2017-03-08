//
//  testFileHelper.h
//  driveEngine
//
//  Created by Aubrey Goodman on 5/5/15.
//
//

#ifndef driveEngine_testFileHelper_h
#define driveEngine_testFileHelper_h

#include "util/helpers/includeGTest.h"
#include "util/fileUtils/fileUtils.h"
#include <fstream>
#include <string>
#include <sys/stat.h>

#define ASSERT_FILE_EXISTS(fileName) \
  ASSERT_PRED1(Anki::Util::FileUtils::FileExists, fileName)

#define ASSERT_FILE_DOES_NOT_EXIST(fileName) \
  ASSERT_PRED1(Anki::Util::FileUtils::FileDoesNotExist, fileName)

#define ASSERT_DIR_EXISTS(dirName) \
  ASSERT_PRED1(Anki::Util::FileUtils::DirectoryExists, dirName)

#define ASSERT_DIR_DOES_NOT_EXIST(dirName) \
  ASSERT_PRED1(Anki::Util::FileUtils::DirectoryDoesNotExist, dirName)

class FileHelper : public ::testing::Test
{
public:
  
  static void GetWorkroot(std::string& path)
  {
    std::string workRoot = "./";
    char* workRootChars = getenv("ANKIWORKROOT");
    if (workRootChars != NULL) {
      workRoot = workRootChars;
    }
    path = workRoot;
  }

  static void DirectoryShouldExist(const std::string& path)
  {
    AssertDirectoryExists(path, true);
  }
  
  static void DirectoryShouldNotExist(const std::string& path)
  {
    AssertDirectoryExists(path, false);
  }
  
  static void FileShouldExist(const std::string& fileName)
  {
    AssertFileExists(fileName, true);
  }
  
  static void FileShouldNotExist(const std::string& fileName)
  {
    AssertFileExists(fileName, false);
  }
  
  static void AssertDirectoryExists(const std::string& path, bool shouldExist)
  {
    struct stat info;
    bool dirExists = false;
    if( stat(path.c_str(), &info)==0 ) {
      if( info.st_mode & S_IFDIR ) {
        dirExists = true;
      }
    }
    if( shouldExist ) {
      ASSERT_TRUE(dirExists) << "Directory '" << path << "' should exist.";
    }
    else {
      ASSERT_FALSE(dirExists) << "Directory '" << path << "' should not exist.";
    }
  }
  
  static void AssertFileExists(const std::string& fileName, bool shouldExist)
  {
    struct stat info;
    bool fileExists = false;
    if( stat(fileName.c_str(), &info)==0 ) {
      fileExists = info.st_mode & S_IFREG;
    }
    if( shouldExist ) {
      ASSERT_TRUE(fileExists) << "'" << fileName << "' should exist.";
    }
    else {
      ASSERT_FALSE(fileExists) << "'" << fileName << "' should not exist.";
    }
  }
  
  static void AssertFileExistsWithContents(const std::string& fileName,
                                           const std::string& contents)
  {
    std::string actualBody;
    std::ifstream in(fileName, std::ios::in | std::ios::binary);
    ASSERT_TRUE(in.good()) << "'" << fileName << "' should exist";

    in.seekg(0, std::ios::end);
    actualBody.resize((size_t) in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&actualBody[0], actualBody.size());
    in.close();
    ASSERT_EQ(contents, actualBody) << "content of '"
                                    << fileName << "' doesn't match expectation.";
  }
  
};

#endif
