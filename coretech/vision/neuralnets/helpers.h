#ifndef __Anki_ObjectDetector_TensorFlow_Helpers_H__
#define __Anki_ObjectDetector_TensorFlow_Helpers_H__

#include <iostream>
#include <sys/stat.h>

#define QUOTE_HELPER(__ARG__) #__ARG__
#define QUOTE(__ARG__) QUOTE_HELPER(__ARG__)

// TODO: Use coretech shared types.h? Or replicate full enum here?
using Result = int;
const Result RESULT_OK = 0;
const Result RESULT_FAIL = 1;

using TimeStamp_t = uint32_t;

// Define local versions of FulLFilePath and FileExists to avoid needing to include 
// all of Anki::Util
static inline std::string FullFilePath(const std::string& path, const std::string& filename)
{
  return path + "/" + filename;
}

static inline bool FileExists(const std::string& filename)
{
  struct stat st;
  const int statResult = stat(filename.c_str(), &st);
  return (statResult == 0);
}

static inline void PRINT_NAMED_ERROR(const char* eventName, const char* format, ...)
{
  const size_t kMaxStringBufferSize = 1024;

  // parse string
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  va_start(args, format);
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  va_end(args);

  // TODO: log it
  //LogError(eventName, keyValues, logString);

  std::cerr << "ERROR: " << eventName << ": " << logString << std::endl;
}

#endif /* __Anki_ObjectDetector_TensorFlow_Helpers_H__ */