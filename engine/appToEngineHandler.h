/**
 * File: appToEngineHandler.h
 *
 * Author: ross
 * Created: Feb 16 2018
 *
 * Description: A *** TEMPORARY *** handler for app messages sent via the webservice to the engine
 *              and vice versa, just so we can work on features for vertical slice before the app
 *              connection flow is complete. This is designed to be as self-contained as possible
 *              TODO: VIC-1398 REMOVE
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Engine_AppToEngineHandler_H__
#define __Cozmo_Engine_AppToEngineHandler_H__

#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <sstream>
#include <vector>


namespace Json {
class Value;
}

namespace Anki {
namespace Cozmo {
  
template <typename Type>
class AnkiEvent;
  
class IExternalInterface;

namespace ExternalInterface {
class MessageEngineToGame;
class MessageGameToEngine;
}
  
namespace WebService {
class WebService;
}

  
class AppToEngineHandler : private Util::noncopyable
{
public:
  AppToEngineHandler();
  
  void Init(WebService::WebService* ws, IExternalInterface* ei);
  
  // webserver will call this with everything after the ? in the url
  std::string ParseAppToEngineRequest( const std::string& params ) const;
  
  std::string GetPendingEngineToAppMessages();
  
private:
  
  // since MessageGameToEngine members are messages, not structures, we can't SetFromJSON and have
  // to do it manually
  bool CreateCLAD( const Json::Value& data, ExternalInterface::MessageGameToEngine& msg ) const;
  
  void HandleEngineToGameMessage( const AnkiEvent<ExternalInterface::MessageEngineToGame>& event );
  
  // assigns str to data with the best guess of type (string, int, or float)
  void AssignBestGuess( Json::Value& data, const std::string& str ) const;
  
  WebService::WebService* _webService = nullptr;
  
  IExternalInterface* _externalInterface = nullptr;
  
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // buffer of accumulated responses
  std::stringstream _pendingEngineToApp;
  unsigned int _pendingCount = 0;
  
  
  
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Engine_AppToEngineHandler_H__
