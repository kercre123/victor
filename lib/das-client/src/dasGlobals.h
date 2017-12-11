/**
 * File: dasGlobals
 *
 * Author: seichert
 * Created: 01/30/15
 *
 * Description: Constants for DAS Globals
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#ifndef __DasGlobals_H__
#define __DasGlobals_H__

namespace Anki
{
namespace Das
{

static constexpr const char* kMessageVersionGlobalKey = "$messv";
static constexpr const char* kMessageVersionGlobalValue = "1";
static constexpr const char* kTimeStampGlobalKey = "$ts";
static constexpr const char* kPhoneTypeGlobalKey = "$phone";
static constexpr const char* kUnitIdGlobalKey = "$unit";
static constexpr const char* kApplicationVersionGlobalKey = "$app";
static constexpr const char* kUserIdGlobalKey = "$user";
static constexpr const char* kApplicationRunGlobalKey = "$apprun";
static constexpr const char* kGameIdGlobalKey = "$game";
static constexpr const char* kMessageLevelGlobalKey = "$level";
static constexpr const char* kGroupIdGlobalKey = "$group";
static constexpr const char* kSequenceGlobalKey = "$seq";
static constexpr const char* kDataGlobalKey = "$data";
static constexpr const char* kPhysicalIdGlobalKey = "$phys";
static constexpr const char* kPlatformGlobalKey = "$platform";
static constexpr const char* kProductGlobalKey = "$product";
static constexpr const char* kPlayerIdGlobalKey = "$player_id";
static constexpr const char* kProfileIdGlobalKey = "$profile_id";
static constexpr const char* kConnectedSessionIdGlobalKey = "$session_id";


} // namespace Das
} // namespace Anki

#endif // __DasGlobals_H__
