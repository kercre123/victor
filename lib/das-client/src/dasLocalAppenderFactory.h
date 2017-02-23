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
#ifndef __DasLocalAppenderFactory_H__
#define __DasLocalAppenderFactory_H__

#include "DAS.h"
#include "DASPrivate.h"
#include <string>
#include <map>

namespace Anki
{
namespace Das
{
class DasLocalAppender;

class DasLocalAppenderFactory {
public:
  // Returns an appender based on the desired mode
  // DASLogMode_System - returns appender that goes to system log
  // DASLogMode_Normal - returns appender that goes to console
  // DASLogMode_Both - returns appender that goes to system log and console
  static DasLocalAppender* CreateAppender(DASLocalLoggerMode logMode);
};

} // namespace Das
} // namespace Anki

#endif // __DasLocalAppenderFactory_H__
