/**
 * File: faceEval.cpp
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: The "main" file to run Face Detection/Identification/Confusion tests. Mostly handles common
 *              argument parsing and configuration tasks.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/vision/tools/faceEval/faceEvalConfusion.h"
#include "coretech/vision/tools/faceEval/faceEvalDetection.h"
#include "coretech/vision/tools/faceEval/faceEvalIdentify.h"
#include "coretech/vision/tools/faceEval/simpleArgParser.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>

#define LOG_CHANNEL "FaceEval"

namespace {
  std::random_device randomDevice;
  std::mt19937 randomEngine(randomDevice());
  const std::vector<const char*> imageFileExts{"jpg", "png"};
  const bool kUseFullPath = true;
}

using namespace Anki;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline std::string GetExtension(const std::string& filename)
{
  const std::string& ext = filename.substr(filename.size()-3,3);
  return ext;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void GetImageFilesInDir(const std::string& rootDir,
                               std::vector<std::string>& filesWithFaces,
                               const bool sampleFilesInSubDirs,
                               const int maxFilesPerSubDir)
{
  using namespace Anki;
  
  std::list<std::string> subDirs{"."};
  {
    std::vector<std::string> tempSubDirs;
    Util::FileUtils::ListAllDirectories(rootDir, tempSubDirs);
    std::copy(tempSubDirs.begin(), tempSubDirs.end(), std::back_inserter(subDirs));
  }
  
  for(const auto& subDir : subDirs)
  {
    if(subDir != ".")
    {
      std::vector<std::string> subSubDirs;
      Util::FileUtils::ListAllDirectories(Util::FileUtils::FullFilePath({rootDir, subDir}), subSubDirs);
      for(const auto& subSubDir : subSubDirs)
      {
        subDirs.push_back(Util::FileUtils::FullFilePath({subDir, subSubDir}));
      }
    }
    
    std::vector<std::string> files = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({rootDir, subDir}), kUseFullPath, imageFileExts);
    
    if(sampleFilesInSubDirs && (files.size() > (size_t)maxFilesPerSubDir))
    {
      std::shuffle(files.begin(), files.end(), randomEngine);
      files.erase(files.begin() + maxFilesPerSubDir, files.end());
    }
    
    std::copy(files.begin(), files.end(), std::back_inserter(filesWithFaces));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static Result GetImageFiles(const std::string& rootDir,
                            const Vision::SimpleArgParser& argParser,
                            Vision::IFaceEval::FileList& filesWithFaces,
                            Vision::IFaceEval::FileList& backgrounds)
{
  int maxFilesPerSubDir = 0;
  const bool sampleFilesInSubDirs = argParser.GetArg("--maxFilesPerSubDir", maxFilesPerSubDir);
  
  std::string specificFile;
  if(argParser.GetArg("--filename", specificFile))
  {
    const std::string& fullPath = Util::FileUtils::FullFilePath({rootDir, specificFile});
    if(GetExtension(fullPath) == "txt")
    {
      std::ifstream file;
      file.open(fullPath, std::ios::in | std::ifstream::binary);
      if( file.is_open() ) {
        std::string line;
        while (std::getline(file, line)) {
          const std::string& lineFullPath = Util::FileUtils::FullFilePath({rootDir, line});
          if(Util::FileUtils::DirectoryExists(lineFullPath))
          {
            GetImageFilesInDir(lineFullPath, filesWithFaces, sampleFilesInSubDirs, maxFilesPerSubDir);
          }
          else
          {
            filesWithFaces.push_back(lineFullPath);
          }
        }
      }
      else
      {
        PRINT_NAMED_ERROR("EvalFace.BadFileList", "%s", specificFile.c_str());
        return RESULT_FAIL;
      }
    }
    else
    {
      if(Util::FileUtils::DirectoryExists(fullPath))
      {
        GetImageFilesInDir(fullPath, filesWithFaces, sampleFilesInSubDirs, maxFilesPerSubDir);
      }
      else
      {
        filesWithFaces.push_back(fullPath);
      }
    }
  }
  else
  {
    GetImageFilesInDir(rootDir, filesWithFaces, sampleFilesInSubDirs, maxFilesPerSubDir);
    
    std::string backgroundDir = "resources/test/markerDetectionTests/NoMarkers"; // assumes running from victor base dir
    argParser.GetArg("--backgroundDir", backgroundDir);
    if(!backgroundDir.empty())
    {
      if(ANKI_VERIFY(Util::FileUtils::DirectoryExists(backgroundDir),
                     "FaceEval.GetImageFiles.BadBackgroundDir", "%s", backgroundDir.c_str()))
      {
        backgrounds = Util::FileUtils::FilesInDirectory(backgroundDir, kUseFullPath, imageFileExts);
      }
    }
    
    // Shuffle _before_ choosing maxTotalFiles!
    bool shuffleImages = false;
    argParser.GetArg("--shuffle", shuffleImages);
    if(shuffleImages)
    {
      std::shuffle(filesWithFaces.begin(), filesWithFaces.end(), randomEngine);
      std::shuffle(backgrounds.begin(), backgrounds.end(), randomEngine);
    }
    
    int maxTotalFiles = 0;
    argParser.GetArg("--maxTotalFiles", maxTotalFiles);
    if(maxTotalFiles > 0)
    {
      if(filesWithFaces.size() > (size_t)maxTotalFiles)
      {
        filesWithFaces.erase(filesWithFaces.begin() + maxTotalFiles, filesWithFaces.end());
      }
      if(backgrounds.size() > (size_t)maxTotalFiles)
      {
        backgrounds.erase(backgrounds.begin() + maxTotalFiles, backgrounds.end());
      }
    }
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char* argv[])
{
  Util::PrintfLoggerProvider printfLoggerProvider;
  printfLoggerProvider.SetMinLogLevel(Util::LOG_LEVEL_DEBUG);
  Util::gLoggerProvider = &printfLoggerProvider;
  
  const Vision::SimpleArgParser argParser(argc, argv);
  
  Vision::IFaceEval::FileList filesWithFaces;
  Vision::IFaceEval::FileList backgrounds;
  
  std::string rootDir;
  if(!argParser.GetArg("--rootDir", rootDir))
  {
    LOG_ERROR("FaceEval.Main.MissingRootDir", "--rootDir arg is required");
    return -1;
  }
  
  bool doDetection = false;
  bool doConfusion = false;
  bool doIdentification = false;
  argParser.GetArg("--detection", doDetection);
  argParser.GetArg("--confusion", doConfusion);
  argParser.GetArg("--identification", doIdentification);
  
  if(doDetection || doConfusion)
  {
    const Result result = GetImageFiles(rootDir, argParser, filesWithFaces, backgrounds);
    if(RESULT_OK != result)
    {
      return -1;
    }
    LOG_INFO("Detection/Confusion FaceFiles", "%zu", filesWithFaces.size());
  }
  
  if(doDetection)
  {
    Vision::FaceEvalDetection evalDetection(filesWithFaces, backgrounds, argParser);
    evalDetection.Run();
  }
  
  if(doConfusion)
  {
    Vision::FaceEvalConfusion evalConfusion(filesWithFaces, argParser);
    evalConfusion.Run();
  }
  
  if(doIdentification)
  {
    std::vector<std::string> tempSubDirs;
    Util::FileUtils::ListAllDirectories(rootDir, tempSubDirs);
    std::list<std::string> subDirs;
    for(const auto& tempSubDir : tempSubDirs)
    {
      const std::string& fullPath = Util::FileUtils::FullFilePath({rootDir, tempSubDir});
      subDirs.push_back(fullPath);
    }
    
    LOG_INFO("Identification SubDirs", "%zu", subDirs.size());
    
    Vision::FaceEvalIdentify evalIdentify(subDirs, argParser);
    evalIdentify.Run();
  }
  
  return 0;
}
