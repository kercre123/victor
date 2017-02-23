/**
 * File: dasLocalAppenderImpl
 *
 * Author: shpung
 * Created: 05/05/14
 *
 * Description: DAS Local Appender.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#ifndef __DasLocalAppenderImpl_H__
#define __DasLocalAppenderImpl_H__

#include "DAS.h"
#include "DASPrivate.h"
#include "dasLocalAppender.h"
#include <string>
#include <map>

namespace Anki
{
namespace Das
{
class DasLocalAppenderImpl : public DasLocalAppender {
public:
  virtual ~DasLocalAppenderImpl() {}
  virtual void append(DASLogLevel level, const char* eventName, const char* eventValue,
                      ThreadId_t threadId, const char* file, const char* funct, int line,
              		  const std::map<std::string,std::string>* globals,
              		  const std::map<std::string,std::string>& data,
              		  const char* globalsAndDataInfo);

  virtual void flush();

};

} // namespace Das
} // namespace Anki

#endif // __DasLocalAppenderImpl_H__
