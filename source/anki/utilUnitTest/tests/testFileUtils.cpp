//
//  testFileUtils.cpp
//  driveEngine
//
//  Created by Aubrey Goodman on 5/5/15.
//
//

#include "util/fileUtils/fileUtils.h"
#include "utilUnitTest/tests/testFileHelper.h"

class FileUtilsTest : public FileHelper
{
public:
  
  static void SetUpTestCase() {}
  
  static void TearDownTestCase() {}

};

TEST_F(FileUtilsTest, WriteDeleteFile)
{
  std::string fileName("testWriteFile");
  Anki::Util::FileUtils::DeleteFile(fileName);
  FileShouldNotExist(fileName);
  
  Anki::Util::FileUtils::WriteFile(fileName, "file contents");
  AssertFileExistsWithContents(fileName, "file contents");

  Anki::Util::FileUtils::DeleteFile(fileName);
  FileShouldNotExist(fileName);
}

TEST_F(FileUtilsTest, CreateRemoveDirectory)
{
  std::string path("/tmp/testCreateDirectory");
  Anki::Util::FileUtils::RemoveDirectory(path);
  DirectoryShouldNotExist(path);
  
  Anki::Util::FileUtils::CreateDirectory(path);
  DirectoryShouldExist(path);
  
  Anki::Util::FileUtils::RemoveDirectory(path);
  DirectoryShouldNotExist(path);

  // test creating intermediate folders
  Anki::Util::FileUtils::CreateDirectory(path+"/A/B/cc", false, true);
  DirectoryShouldExist(path+"/A/B/cc");

  Anki::Util::FileUtils::RemoveDirectory(path);
  DirectoryShouldNotExist(path);

  // test creating intermediate folders with file name included
  Anki::Util::FileUtils::CreateDirectory(path+"/A/B/cc/myfile.txt", true, true);
  DirectoryShouldExist(path+"/A/B/cc");

  Anki::Util::FileUtils::RemoveDirectory(path);
  DirectoryShouldNotExist(path);

  // test creating folder with file name included
  Anki::Util::FileUtils::CreateDirectory(path+"A/myfile.txt", true, false);
  DirectoryShouldExist(path+"A");

  Anki::Util::FileUtils::RemoveDirectory(path+"A");
  DirectoryShouldNotExist(path+"A");

}

TEST_F(FileUtilsTest, ListDirectory)
{
  std::string path("/tmp/testListDirectory");
  Anki::Util::FileUtils::RemoveDirectory(path);
  DirectoryShouldNotExist(path);
  
  Anki::Util::FileUtils::CreateDirectory(path);
  DirectoryShouldExist(path);
  
  std::string fileName("testListDirectoryFile");
  Anki::Util::FileUtils::WriteFile(path + "/" + fileName, "file contents");
  FileShouldExist(path + "/" + fileName);
  
  std::vector<std::string> files = Anki::Util::FileUtils::FilesInDirectory(path);
  ASSERT_EQ(1, files.size());
  ASSERT_EQ(fileName, files[0]);
  
  // Check that useFullPath works
  files = Anki::Util::FileUtils::FilesInDirectory(path, true);
  ASSERT_EQ(1, files.size());
  ASSERT_EQ(path + "/" + fileName, files[0]);
  
  // Check that useFullPath works when path already has trailing file separator
  files = Anki::Util::FileUtils::FilesInDirectory(path + "/", true);
  ASSERT_EQ(1, files.size());
  ASSERT_EQ(path + "/" + fileName, files[0]);
  
  Anki::Util::FileUtils::DeleteFile(path + "/" + fileName);
  FileShouldNotExist(path + "/" + fileName);
  
  Anki::Util::FileUtils::RemoveDirectory(path);
  DirectoryShouldNotExist(path);
}

TEST_F(FileUtilsTest, ListDirectoryWithExtensionFilters)
{
  std::string path("/tmp/testListDirectoryWithExtensions");
  Anki::Util::FileUtils::RemoveDirectory(path);
  DirectoryShouldNotExist(path);
  
  Anki::Util::FileUtils::CreateDirectory(path);
  DirectoryShouldExist(path);

  auto FilesMatch = [](const std::set<std::string>& groundTruth, const std::vector<std::string>& foundFiles)
  {
    std::set<std::string> foundFilesSet;
    for(auto filename : foundFiles) {
      foundFilesSet.insert(filename);
    }
    return groundTruth == foundFilesSet;
  };
  
  const std::set<std::string> fileNames {
    "jpegExtension.jpeg",
    "jpgLowerExtension.jpg",
    "jpgUpperExtension.JPG",
    "noExtension",
    "pngExtension.png"
  };
  
  // Create all the files
  for(auto & fileName : fileNames)
  {
    Anki::Util::FileUtils::WriteFile(path + "/" + fileName, "file contents");
    FileShouldExist(path + "/" + fileName);
  }
  
  std::vector<std::string> foundFiles;
  foundFiles = Anki::Util::FileUtils::FilesInDirectory(path);
  ASSERT_TRUE(FilesMatch(fileNames, foundFiles));
  
  foundFiles = Anki::Util::FileUtils::FilesInDirectory(path, false, nullptr);
  ASSERT_TRUE(FilesMatch(fileNames, foundFiles));
  
  foundFiles = Anki::Util::FileUtils::FilesInDirectory(path, false, "");
  ASSERT_TRUE(FilesMatch(fileNames, foundFiles));
  
  foundFiles = Anki::Util::FileUtils::FilesInDirectory(path, false, "jpg");
  std::set<std::string> correctAnswer = {"jpgLowerExtension.jpg"};
  ASSERT_TRUE(FilesMatch(correctAnswer, foundFiles));

  foundFiles = Anki::Util::FileUtils::FilesInDirectory(path, false, {"jpg", "JPG"});
  correctAnswer = {"jpgLowerExtension.jpg", "jpgUpperExtension.JPG"};
  ASSERT_TRUE(FilesMatch(correctAnswer, foundFiles));

  foundFiles = Anki::Util::FileUtils::FilesInDirectory(path, false, {"jpg", "JPG", "jpeg"});
  correctAnswer = {"jpegExtension.jpeg", "jpgLowerExtension.jpg", "jpgUpperExtension.JPG"};
  ASSERT_TRUE(FilesMatch(correctAnswer, foundFiles));
  
  foundFiles = Anki::Util::FileUtils::FilesInDirectory(path, false, {"jpg", "png", "jpeg", "JPG"});
  correctAnswer = {"jpegExtension.jpeg", "jpgLowerExtension.jpg", "jpgUpperExtension.JPG", "pngExtension.png"};
  ASSERT_TRUE(FilesMatch(correctAnswer, foundFiles));

  foundFiles = Anki::Util::FileUtils::FilesInDirectory(path, false, {"jpg", "jpeg"});
  correctAnswer = {"jpegExtension.jpeg", "jpgLowerExtension.jpg"};
  ASSERT_TRUE(FilesMatch(correctAnswer, foundFiles));
  
  // Delete all the files
  for(auto & fileName : fileNames)
  {
    Anki::Util::FileUtils::DeleteFile(path + "/" + fileName);
    FileShouldNotExist(path + "/" + fileName);
  }
  
  // Delete the directory
  Anki::Util::FileUtils::RemoveDirectory(path);
  DirectoryShouldNotExist(path);
}

TEST_F(FileUtilsTest, FileExists)
{
  std::string fileName("testFileExists");
  Anki::Util::FileUtils::DeleteFile(fileName);
  FileShouldNotExist(fileName);
  ASSERT_FALSE(Anki::Util::FileUtils::FileExists(fileName)) << fileName << " should not exist";

  Anki::Util::FileUtils::WriteFile(fileName, "file contents");
  AssertFileExistsWithContents(fileName, "file contents");
  ASSERT_TRUE(Anki::Util::FileUtils::FileExists(fileName)) << fileName << " should exist";

  Anki::Util::FileUtils::DeleteFile(fileName);
  FileShouldNotExist(fileName);

  Anki::Util::FileUtils::CreateDirectory(fileName);
  DirectoryShouldExist(fileName);
  ASSERT_FALSE(Anki::Util::FileUtils::FileExists(fileName)) << fileName << " is a directory";

  Anki::Util::FileUtils::RemoveDirectory(fileName);
  DirectoryShouldNotExist(fileName);
}

TEST_F(FileUtilsTest, RecurseGetFiles)
{
  std::string fileName("testWriteFile.txt");
  std::string path = "/tmp/testRecurseDirectory/";
  
  Anki::Util::FileUtils::CreateDirectory(path);
  DirectoryShouldExist(path);
  
  // Write one version of the file in the base directory
  Anki::Util::FileUtils::WriteFile(path + fileName, "file contents");
  
  std::string subPath = "A/B/cc/";
  
  Anki::Util::FileUtils::CreateDirectory(path + subPath, false, true);
  DirectoryShouldExist(path + subPath);
  
  Anki::Util::FileUtils::WriteFile(path + subPath + fileName, "file contents");
  
  // Find files with recursion
  std::vector<std::string> foundFiles = Anki::Util::FileUtils::FilesInDirectory(path,true,nullptr,true);
  
  for (auto& file : foundFiles)
  {
    FileShouldExist(file);
  }
  
  Anki::Util::FileUtils::RemoveDirectory(path);
  DirectoryShouldNotExist(path);
}

TEST_F(FileUtilsTest, FullFilePath)
{
# ifdef _WIN32
# define FILESEP "\\"
# else
# define FILESEP "/"
# endif
  
  const std::string groundTruth1("path" FILESEP "to" FILESEP "given" FILESEP "file.jpg");
  const std::string groundTruth1b(FILESEP "path" FILESEP "to" FILESEP "given" FILESEP "file.jpg");
  const std::string groundTruth2("path" FILESEP "file.jpg");
  const std::string groundTruth3("file.jpg");

  //
  // Simple concatenation
  //
  std::string result = Anki::Util::FileUtils::FullFilePath({"path", "to", "given", "file.jpg"});
  ASSERT_EQ(result, groundTruth1);
  
  const std::string path1("path"), path2("to"), path3("given"), file("file.jpg");
  result = Anki::Util::FileUtils::FullFilePath({path1, path2, path3, file});
  ASSERT_EQ(result, groundTruth1);
  
  result = Anki::Util::FileUtils::FullFilePath({path1, file});
  ASSERT_EQ(result, groundTruth2);
  
  result = Anki::Util::FileUtils::FullFilePath({"file.jpg"});
  ASSERT_EQ(result, groundTruth3);
  
  result = Anki::Util::FileUtils::FullFilePath({});
  ASSERT_TRUE(result.empty());
  
  //
  // Tests for when strings already have file separators in them
  //
  result = Anki::Util::FileUtils::FullFilePath({"path" FILESEP, FILESEP "to", "given", FILESEP "file.jpg"});
  ASSERT_EQ(result, groundTruth1);
  
  result = Anki::Util::FileUtils::FullFilePath({FILESEP "path" FILESEP, FILESEP "to", FILESEP "given" FILESEP, FILESEP "file.jpg"});
  ASSERT_EQ(result, groundTruth1b);
  
  //
  // Tests for when some already-concatenated subpaths are passed in
  //
  result = Anki::Util::FileUtils::FullFilePath({"path" FILESEP "to", "given" FILESEP "file.jpg"});
  ASSERT_EQ(result, groundTruth1);
  
  result = Anki::Util::FileUtils::FullFilePath({FILESEP "path" FILESEP "to" FILESEP, FILESEP "given" FILESEP "file.jpg"});
  ASSERT_EQ(result, groundTruth1b);

  //
  // Empty string should not insert a FILESEP!
  //
  result = Anki::Util::FileUtils::FullFilePath({"", "file.jpg"});
  ASSERT_EQ(result.front(), 'f');
  
  result = Anki::Util::FileUtils::FullFilePath({"", "", "", "file.jpg"});
  ASSERT_EQ(result.front(), 'f');
  
  result = Anki::Util::FileUtils::FullFilePath({"", "path" FILESEP, "", "file.jpg"});
  ASSERT_EQ(result, groundTruth2);
  
}

TEST_F(FileUtilsTest, RemoveExtension)
{
# ifdef _WIN32
# define FILESEP "\\"
# else
# define FILESEP "/"
# endif
  
  const std::string path1("path" FILESEP "to" FILESEP "given" FILESEP "file.jpg");
  const std::string path2(FILESEP "path" FILESEP "to" FILESEP "given" FILESEP "file.jpg");
  const std::string path3("path" FILESEP "file.jpg");
  const std::string path4("file.jpg");
  const std::string path5("path" FILESEP "to" FILESEP "given" FILESEP "file");
  
  const std::string groundTruth("file");
  
  std::string result = Anki::Util::FileUtils::GetFileName(path1, true, true);
  ASSERT_EQ(result, groundTruth);
  
  result = Anki::Util::FileUtils::GetFileName(path2, true, true);
  ASSERT_EQ(result, groundTruth);
  
  result = Anki::Util::FileUtils::GetFileName(path3, true, true);
  ASSERT_EQ(result, groundTruth);
  
  // There is no separator in path4 so an empty string is returned
  // TODO: Instead of returning an empty string this should probably return "file"
  result = Anki::Util::FileUtils::GetFileName(path4, true, true);
  ASSERT_EQ(result, "");
  
  // path5 does not have an extension so an empty string is returned
  result = Anki::Util::FileUtils::GetFileName(path5, true, true);
  ASSERT_EQ(result, "");
  
  result = Anki::Util::FileUtils::GetFileName(path5, false, true);
  ASSERT_EQ(result, groundTruth);
}
