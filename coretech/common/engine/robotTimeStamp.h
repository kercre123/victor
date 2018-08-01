/**
 * File: robotTimeStamp.h
 *
 * Author: ross
 * Date:   Jun 9 2018
 *
 * Description: Type definition for robot timestamp (a strongly typed TimeStamp_t)
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef _ANKICORETECH_COMMON_SHARED_ROBOTTIMESTAMP_H_
#define _ANKICORETECH_COMMON_SHARED_ROBOTTIMESTAMP_H_
#pragma once

#include "coretech/common/shared/types.h"
#include "util/helpers/stronglyTyped.h"

namespace Anki {

typedef Util::StronglyTyped<TimeStamp_t, struct RobotTimeStampID>  RobotTimeStamp_t;

} // namespace

#endif // _ANKICORETECH_COMMON_SHARED_ROBOTTIMESTAMP_H_
