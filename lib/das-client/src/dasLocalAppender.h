/**
 * File: dasLocalAppender
 *
 * Author: shpung
 * Created: 05/05/14
 *
 * Description: DAS Local Appender.  To be implemented on a
 * platform by platform basis.  Intention is to output to
 * the local console or logcat.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#ifndef __DasLocalAppender_H__
#define __DasLocalAppender_H__

#include "DAS.h"
#include "DASPrivate.h"
#include <string>
#include <map>

namespace Anki
{
namespace Das
{
class DasLocalAppender {
public:
  virtual ~DasLocalAppender() {}
  virtual void append(DASLogLevel level, const char* eventName, const char* eventValue,
                      ThreadId_t threadId, const char* file, const char* funct, int line,
              		  const std::map<std::string,std::string>* globals,
              		  const std::map<std::string,std::string>& data,
              		  const char* globalsAndDataInfo) = 0;

  virtual void flush() = 0;

};

} // namespace Das
} // namespace Anki

#endif // __DasLocalAppender_H__
