/**
 * File: dasLocalAppenderFactory
 *
 * Author: jliptak
 * Created: 08/16/2015
 *
 * Description: Per platform that know of the different types of loggers supported
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "dasLocalAppenderFactory.h"
#include "dasLocalAppenderImpl.h"

namespace Anki
{
namespace Das
{

DasLocalAppender* DasLocalAppenderFactory::CreateAppender(DASLocalLoggerMode /*logMode*/) {
    return new DasLocalAppenderImpl();
}

} // namespace Das
} // namespace Anki
