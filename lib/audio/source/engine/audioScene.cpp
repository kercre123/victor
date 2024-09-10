//
//  audioScene.cpp
//  Audio Engine
//
//  Created by Jordan Rivas on 7/6/15.
//
//

#include "audioEngine/audioScene.h"
#include "json/json.h"
#include "util/logging/logging.h"

namespace Anki {
namespace AudioEngine {
  
// Audio Scene Keys
const char* kSceneNameKey = "SceneName";
const char* kEventsKey = "Events";
const char* kBanksKey = "Banks";
const char* kZipFilesKey = "ZipFiles";
// Audio Event Keys
const char* kEventNameKey = "EventName";
const char* kSwitchStatesKey = "SwitchStates";
const char* kGameStatesKey = "GameStates";
const char* kGameParametersKey = "GameParameters";
// Audio Scene State Group Keys
const char* kStateGroupNameKey = "StateGroupName";
const char* kGroupStatesKey = "GroupStates";
// Audio Parameter Keys
const char* kParameterNameKey = "ParameterName";
const char* kParameterValueKey = "ParameterValue";

  
using namespace Json;

//----------------------------------------------------------------------------------------------------------------------
AudioScene::AudioScene( const std::string& sceneName,
                        const AudioEventList& events,
                        const AudioBankList& banks,
                        const AudioZipFileList& zipFiles )
: SceneName( sceneName )
, Events( events )
, Banks( banks )
, ZipFiles( zipFiles )
{}
  
AudioScene::AudioScene( const Json::Value& sceneJson )
{
  ASSERT_NAMED(sceneJson.isObject(), "AudioScene JSON is not an Object");
  
  const Value& nameVal = sceneJson[kSceneNameKey];
  ASSERT_NAMED(nameVal.isString(), "AudioScene JSON doesn't have Scene Name");
  SceneName = nameVal.asCString();
  
  const Value& eventsVal = sceneJson[kEventsKey];
  if ( !eventsVal.isNull() ) {
    ASSERT_NAMED(eventsVal.isArray(), "AudioScene Event JSON is not Array");
    
    const size_t listSize = eventsVal.size();
    Events.reserve( listSize );
    for ( int idx = 0; idx < listSize; ++idx ) {
      Events.push_back( AudioSceneEvent( eventsVal[idx] ) );
    }
  }
  
  const Value& banksVal = sceneJson[kBanksKey];
  if ( !banksVal.isNull() ) {
    ASSERT_NAMED(banksVal.isArray(), "AudioScene Banks JSON is not Array");
    
    const size_t bankSize = banksVal.size();
    Banks.reserve( bankSize );
    for ( int idx = 0; idx < bankSize; ++idx ) {
      Banks.push_back( banksVal[idx].asString() );
    }
  }
  
  const Value& zipFilesVal = sceneJson[kZipFilesKey];
  if ( !zipFilesVal.isNull() ) {
    ASSERT_NAMED(zipFilesVal.isArray(), "AudioScene Zip Files JSON is not Array");
    
    const size_t zipFilesSize = zipFilesVal.size();
    ZipFiles.reserve( zipFilesSize );
    for ( int idx = 0; idx < zipFilesSize; ++idx ) {
      ZipFiles.push_back( zipFilesVal[idx].asString() );
    }
  }
}

const std::vector<std::string> AudioScene::GetSwitchGroups( const std::string& eventName )
{
  std::vector<std::string> groups = std::vector<std::string>();

  if (eventName.length() == 0) {
    // All Event Switch States
    for ( auto& event : Events ) {
      std::vector<std::string> states = event.GetSwitchStates();
      groups.insert( groups.end(), states.begin(), states.end() );
    }
  }
  else {
    // Specific Event
    const auto& it = std::find_if( Events.begin(),
                                   Events.end(),
                                   [&eventName](const AudioSceneEvent& event) {return  event.EventName == eventName;} );
    if (it != Events.end()) {
      // Found it!
      std::vector<std::string> states = it->GetSwitchStates();
      groups.insert( groups.end(), states.begin(), states.end() );
    }
  }

  return groups;
}


//----------------------------------------------------------------------------------------------------------------------

AudioSceneEvent::AudioSceneEvent( const std::string& eventName,
                                  const AudioGameSyncGroupList& switchStates,
                                  const AudioGameSyncGroupList& gameStates,
                                  const AudioParameterList& gameParameters )
: EventName( eventName )
, SwitchStates( switchStates )
, GameStates( gameStates )
, Parameters( gameParameters )
{}
  
AudioSceneEvent::AudioSceneEvent( const Json::Value& eventJson )
{
  ASSERT_NAMED(eventJson.isObject(), "AudioSceneEvent JSON is not an Object");
  
  // Evnet
  const Value& nameVal = eventJson[kEventNameKey];
  ASSERT_NAMED(nameVal.isString(), "AudioSceneEvent JSON doesn't have Event Name");
  EventName = nameVal.asString();
  
  // Switch States
  const Value& switchStatesVal = eventJson[kSwitchStatesKey];
  if ( !switchStatesVal.isNull() ) {
    ASSERT_NAMED(switchStatesVal.isArray(), "AudioSceneEvent SwitchStates JSON is not an Array");
    
    const size_t listSize = switchStatesVal.size();
    SwitchStates.reserve( listSize );
    for ( int idx = 0; idx < listSize; ++idx ) {
      SwitchStates.push_back( AudioSceneStateGroup( switchStatesVal[idx] ) );
    }
  }
  
  // Game States
  const Value& gameStatesVal = eventJson[kGameStatesKey];
  if ( !gameStatesVal.isNull() ) {
    ASSERT_NAMED(gameStatesVal.isArray(), "AudioSceneEvent GameStates JSON is not an Array");
    
    const size_t listSize = gameStatesVal.size();
    GameStates.reserve( listSize );
    for ( int idx = 0; idx < listSize; ++idx ) {
      GameStates.push_back( AudioSceneStateGroup( gameStatesVal[idx] ) );
    }
  }
  
  // Parameters
  const Value& gameParameterVal = eventJson[kGameParametersKey];
  if ( !gameParameterVal.isNull() ) {
    ASSERT_NAMED(gameParameterVal.isArray(), "AudioSceneEvent GameParameters JSON is not an Array");
    
    const size_t listSize = gameParameterVal.size();
    Parameters.reserve( listSize );
    for ( int idx = 0; idx < listSize; ++idx) {
      Parameters.push_back( AudioSceneParameter( gameParameterVal[idx] ) );
    }
  }
}

const std::vector<std::string> AudioSceneEvent::GetSwitchStates() {
  std::vector<std::string> switches = std::vector<std::string>();
  for ( auto& aSwitch : SwitchStates ) {
    switches.push_back( aSwitch.StateGroupName );
  }
  return switches;
}


//----------------------------------------------------------------------------------------------------------------------
  
AudioSceneStateGroup::AudioSceneStateGroup( const std::string& stateGroupName, const AudioGroupStateList& defaultStates )
: StateGroupName( stateGroupName )
, GroupStates( defaultStates )
{}
  
AudioSceneStateGroup::AudioSceneStateGroup( const Json::Value& groupJson )
{
  ASSERT_NAMED(groupJson.isObject(), "AudioSceneStateGroup JSON is not an Object");
  
  const Value& nameVal = groupJson[kStateGroupNameKey];
  ASSERT_NAMED(nameVal.isString(), "AudioSceneStateGroup JSON doesn't have State Group Name");
  StateGroupName = nameVal.asString();
  
  const Value& groupStatesVal = groupJson[kGroupStatesKey];
  ASSERT_NAMED(groupStatesVal.isArray(), "AudioSceneStateGroup Group States JSON is not an Array");
  
  const size_t listSize = groupStatesVal.size();
  GroupStates.reserve( listSize );
  for ( int idx = 0; idx < listSize; ++idx ) {
    GroupStates.push_back( groupStatesVal[idx].asString() );
  }
}
  
  
//----------------------------------------------------------------------------------------------------------------------

AudioSceneParameter::AudioSceneParameter( const std::string& parameterName, float parameterValue )
: ParameterName( parameterName )
, ParameterValue( parameterValue )
{}

AudioSceneParameter::AudioSceneParameter( const Json::Value& parameterJson )
{
  ASSERT_NAMED(parameterJson.isObject(), "AudioSceneParameter JSON is not an Object");
  
  const Value& nameVal = parameterJson[kParameterNameKey];
  ASSERT_NAMED(nameVal.isString(), "AudioSceneParameter JSON doesn't have a Parameter Name");
  ParameterName = nameVal.asString();
  
  const Value& valueVal = parameterJson[kParameterValueKey];
  ASSERT_NAMED(valueVal.isNumeric(), "AudioSceneParameter JSON doesn't have a Parameter Value");
  ParameterValue = valueVal.asFloat();
}

} // AudioEngine
} // Anki
