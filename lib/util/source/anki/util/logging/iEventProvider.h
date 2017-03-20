/**
* File: iEventProvider
*
* Author: damjan
* Created: 4/21/2015
*
* Description: interface for anki BI Events
*
* Copyright: Anki, Inc. 2014
*
**/

#ifndef __Util_Logging_IEventProvider_H__
#define __Util_Logging_IEventProvider_H__

#include <map>
#include <string>

namespace Anki {
namespace Util {

class IEventProvider {
public:
  // print event should be part of this interface
  // but in our case it makes more sense to keep it in the iLoggerProvider
  //virtual void PrintEvent(const char* eventName,
  //  const std::vector<std::pair<const char*, const char*>>& keyValues,
  //  const char* eventValue) = 0;

  // sets global properties.
  // all future logs+events will have the updated globals attached to them
  virtual void SetGlobal(const char* key, const char* value) = 0;

  // Get Globals
  virtual void GetGlobals(std::map<std::string, std::string>& dasGlobals) = 0;

  virtual void EnableNetwork(int reason) {}
  virtual void DisableNetwork(int reason) {}

};




} // namespace Util
} // namespace Anki

#endif // __Util_Logging_IEventProvider_H__

