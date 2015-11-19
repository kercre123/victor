/**
 * File: DebugConsoleManager
 *
 * Author: Molly Jameson
 * Created: 11/17/15
 *
 * Description: A singleton wrapper around the console so that it can use CLAD at
 * the game level instead of util. If you need to specify robotID for a console command it needs to be a function.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_Debug_DebugConsoleManager_H__
#define __Cozmo_Basestation_Debug_DebugConsoleManager_H__


namespace Anki {
namespace Cozmo {


  
template <typename Type>
class AnkiEvent;


namespace ExternalInterface {
  class MessageGameToEngine;
}
class CozmoEngineHostImpl;
  
class DebugConsoleManager
{
//----------------------------------------------------------------------------------------------------------------------------
public:
  void Init( CozmoEngineHostImpl* engine );
  void HandleEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
//----------------------------------------------------------------------------------------------------------------------------
private:
  void SendAllDebugConsoleVars();
  CozmoEngineHostImpl* _engine;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Debug_DebugConsoleManager_H__

