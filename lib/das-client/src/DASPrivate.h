//
//  DASPrivate.h
//  DAS
//
//  Created by Mark Pauley on 6/14/13.
//  Copyright (c) 2013 Anki. All rights reserved.
//

#ifndef DAS_DASPrivate_h
#define DAS_DASPrivate_h

#include <map>
#include <string>
#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif

extern int DASNetworkingDisabled;
namespace Anki {
namespace Das {
class DasGameLogAppender;
using ThreadId_t = std::uint32_t;
static const size_t kDefaultMaxLogLength = 100 * 1024;
static const size_t kDasDefaultMaxLogFiles = 400;
// This is the max in order to fit within amazon SQS limit, which is 256000 bytes
// We need to stay within ~185K in order for the base64 encoded size to fit within the limit
static constexpr size_t kDasMaxAllowableLogLength = 185 * 1024;
} // namespace DAS
} // namespace Anki

void getDASLogLevelName(DASLogLevel logLevel, std::string& outLogLevelName);

void getDASTimeString(std::string& outTimeString);

void _DAS_GetGlobal(const char* key, std::string& outValue);

void _DAS_ClearGlobals();

Anki::Das::DasGameLogAppender* _DAS_GetGameLogAppender(); // used for unit tests

void getDASGlobalsAndDataString(const std::map<std::string,std::string>* globals,
                                const std::map<std::string,std::string>& data,
                                std::string& outGlobalsAndData);

#ifdef __cplusplus
}
#endif

#endif
