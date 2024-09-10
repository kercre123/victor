/*
 * File: audioActionTypes.h
 *
 * Author: Jordan Rivas
 * Created: 06/27/2018
 *
 * Description: Set of class and structs to query Maya and store audio keyframe data.
 *
 * Copyright: Anki, Inc. 2018
 */


#include "audioActionTypes.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"

namespace {
// Need static RNG
Anki::Util::RandomGenerator kRng;
}

// Useful function to check what a Maya queried data looks like
template <class ARRAY_TYPE>
MString CreateString(ARRAY_TYPE& array)
{
  MString str("[ ");
  for (uint idx = 0; idx < array.length(); ++idx) {
    if (0 != idx) {
      str += ", ";
    }
    str += array[idx];
  }
  str += " ]";
  return str;
}

// ---------------------------------------------------------------------------------------------------------------------
// Event Action
// ---------------------------------------------------------------------------------------------------------------------

EventAction::EventAction( std::string name, float probability, float volume )
: name( name ), probability( probability ), volume( volume ) {}

const std::string EventAction::Description() const
{
  return name + " - p: " + std::to_string(probability) + " v: " + std::to_string(volume);
}

const std::string StateAction::Description() const
{
  return stateGroup + " - " + state;
}

const std::string SwitchAction::Description() const
{
  return switchGroup + " - " + state;
}

const std::string ParameterAction::Description() const
{
  return name + " - v: " + std::to_string(value) + " d: " + std::to_string(duration_ms) + " c: " + std::to_string(curveType);
}

// ---------------------------------------------------------------------------------------------------------------------
// Audio Keyframe
// ---------------------------------------------------------------------------------------------------------------------
const std::string AudioKeyframe::Description() const
{
  std::stringstream ss;
  if ( !events.empty() ) {
    ss << "---------- Events ----------" << std::endl;
    for ( const auto& event : events ) {
      ss << event.Description() << std::endl;
    }
  }
  
  if ( !states.empty() ) {
    ss << "---------- States ----------";
    for ( const auto& state : states ) {
      ss << state.Description() << std::endl;
    }
  }
  
  if ( !switches.empty() ) {
    ss << "---------- Switches ----------";
    for ( const auto& gameSwitch : switches ) {
      ss << gameSwitch.Description() << std::endl;
    }
  }
  
  if ( !parameters.empty() ) {
    ss << "---------- Parameters ----------";
    for ( const auto& param : parameters ) {
      ss << param.Description() << std::endl;
    }
  }
  return ss.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<> std::vector<EventAction>& AudioKeyframe::GetTypeVector()
{
  return events;
}

template<> std::vector<StateAction>& AudioKeyframe::GetTypeVector()
{
  return states;
}

template<> std::vector<SwitchAction>& AudioKeyframe::GetTypeVector()
{
  return switches;
}

template<> std::vector<ParameterAction>& AudioKeyframe::GetTypeVector()
{
  return parameters;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const EventAction* AudioKeyframe::GetAudioEventToPlay() const
{
  
  uint numEvents = events.size();
  if (numEvents == 0) {
    return nullptr;
  }
  
  auto randVal = kRng.RandDbl();
  auto currentProbVal = 0.0f;
  for ( const auto& anEvent : events ) {
    if ((anEvent.probability + currentProbVal) > randVal) {
      // Winner, play this event
      return &anEvent;
    }
    currentProbVal += anEvent.probability;
  }
  
  return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// QueryDef
// ---------------------------------------------------------------------------------------------------------------------
QueryDef::QueryDef(const std::string attribute, const std::string queryType, MDoubleArray&& container )
: attribute( attribute )
, queryType( queryType )
, type ( ContainerType::Double )
, doubleContainer( container ) { }

QueryDef::QueryDef(const std::string attribute, const std::string queryType, MIntArray&& container )
: attribute( attribute )
, queryType( queryType )
, type ( ContainerType::Int )
, intContainer( container ) { }

QueryDef::QueryDef(const std::string attribute, const std::string queryType, MStringArray&& container )
: attribute( attribute )
, queryType( queryType )
, type ( ContainerType::String )
, stringContainer( container ) { }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QueryDef::QueryDef( const QueryDef& other )
{
  attribute = other.attribute;
  queryType = other.queryType;
  type = other.type;
  switch ( type ) {
    case ContainerType::Double:
      new (&doubleContainer) auto(other.doubleContainer);
      break;
    case ContainerType::Int:
      new (&intContainer) auto(other.intContainer);
      break;
    case ContainerType::String:
      new (&stringContainer) auto(other.stringContainer);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QueryDef& QueryDef::operator=( const QueryDef& other )
{
  if (&other == this) {
    return *this;
  }
  attribute = other.attribute;
  queryType = other.queryType;
  type = other.type;
  switch ( type ) {
    case ContainerType::Double:
      doubleContainer = other.doubleContainer;
      break;
    case ContainerType::Int:
      intContainer = other.intContainer;
      break;
    case ContainerType::String:
      stringContainer = other.stringContainer;
      break;
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QueryDef::PerformQuery( uint nodeIdx )
{
  // Do nothing if the audio node (x:AnkiAudioNode) doesn't exist
  int nodeExists;
  MString existCmd("objExists \"x:AnkiAudioNode\"");
  MGlobal::executeCommand( existCmd, nodeExists );
  if (nodeExists != 1) {
    return;
  }

  // Static query strings
  static const std::string cmdAttribute   = "keyframe -attribute \"";
  static const std::string cmdQuery = "\" -query -";
  static const std::string cmdNode = " \"x:AnkiAudioNode\"";
  std::string attributeIdx;
  
  if ( nodeIdx > 0 ) {
    // Don't add index to first idx
    attributeIdx = std::to_string( nodeIdx + 1 );
  }
  
  const std::string queryCmd = cmdAttribute + attribute + attributeIdx + cmdQuery + queryType + cmdNode;
  const MString cmd( queryCmd.c_str() );
  
  switch ( type ) {
    case QueryDef::ContainerType::Double:
      executeStatus = MGlobal::executeCommand( cmd, doubleContainer );
      break;
      
    case QueryDef::ContainerType::Int:
      executeStatus = MGlobal::executeCommand( cmd, intContainer );
      break;
      
    case QueryDef::ContainerType::String:
      executeStatus = MGlobal::executeCommand( cmd, stringContainer );
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QueryDef::Clear()
{
  switch (type) {
    case ContainerType::Double:
      doubleContainer.clear();
      break;
    case ContainerType::Int:
      intContainer.clear();
      break;
    case ContainerType::String:
      stringContainer.clear();
      break;
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// Audio Action Capture Types
// ---------------------------------------------------------------------------------------------------------------------

namespace {
static const float kInvalidAtt = -1.0f;
}

EventActionCapture::EventActionCapture()
{
  const std::string eventAtt          = "WwiseIdEnum";
  const std::string probabilityAtt    = "probability";
  const std::string volumeAtt         = "volume";
  const std::string timeChangeQuery   = "timeChange";
  const std::string valueChangeQuery  = "valueChange";
  queryMap.emplace( "wwiseIdTime", QueryDef( eventAtt, timeChangeQuery, MIntArray() ) );
  queryMap.emplace( "wwiseIdValue", QueryDef( eventAtt, valueChangeQuery, MIntArray() ) );
  queryMap.emplace( "probTime", QueryDef( probabilityAtt, timeChangeQuery, MIntArray() ) );
  queryMap.emplace( "probValue", QueryDef( probabilityAtt, valueChangeQuery, MDoubleArray() ) );
  queryMap.emplace( "volumeTime", QueryDef( volumeAtt, timeChangeQuery, MIntArray() ) );
  queryMap.emplace( "volumeValue", QueryDef( volumeAtt, valueChangeQuery, MDoubleArray() ) );
  enumListMap.emplace( eventAtt, MStringArray() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventActionCapture::validQuery() {
  const auto& wwiseIdTimeLen = queryMap.find("wwiseIdTime")->second.intContainer.length();
  const auto& wwiseIdValsLen  = queryMap.find("wwiseIdValue")->second.intContainer.length();
  return (wwiseIdTimeLen > 0) &&
  (wwiseIdValsLen > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventActionCapture::ComputeResults() {
  
  // Get event Timestamp
  const auto& eventTimes          = queryMap.find("wwiseIdTime");
  const auto& eventVals           = queryMap.find("wwiseIdValue");
  const auto& probTimes           = queryMap.find("probTime");
  const auto& probVals            = queryMap.find("probValue");
  const auto& volumeTimes         = queryMap.find("volumeTime");
  const auto& volumeVals          = queryMap.find("volumeValue");
  const auto& eventTimeContainer  = eventTimes->second.intContainer;
  const auto& eventValContainer   = eventVals->second.intContainer;
  const auto& probTimeContainer   = probTimes->second.intContainer;
  const auto& probValContainer    = probVals->second.doubleContainer;
  const auto& volumeTimeContainer = volumeTimes->second.intContainer;
  const auto& volumeValContainer  = volumeVals->second.doubleContainer;
  
  // Validate event data
  if ( eventTimeContainer.length() != eventValContainer.length() ) {
    MGlobal::displayError("ComputeResults - Audio keyframes out of sync");
    return;
  }
  
  // Check if there is probability and there is
  std::map<KeyframeTime, float> probMap;
  if ( probTimeContainer.length() == probValContainer.length() ) {
    // Build map
    for ( size_t idx = 0; idx < probTimeContainer.length(); ++idx ) {
      const KeyframeTime time = static_cast<KeyframeTime>( probTimeContainer[idx] );
      probMap[time] = static_cast<float>(probValContainer[idx] / 100.0); // normalize to [0, 1]
    }
  }
  else {
    MGlobal::displayWarning("ComputeResults - Audio event probabilities out of sync, so ignoring them");
  }
  
  std::map<KeyframeTime, float> volumeMap;
  if ( volumeTimeContainer.length() == volumeValContainer.length() ) {
    // Build map
    for ( size_t idx = 0; idx < volumeTimeContainer.length(); ++idx ) {
      const KeyframeTime time = static_cast<KeyframeTime>( volumeTimeContainer[idx] );
      volumeMap[time] = static_cast<float>(volumeValContainer[idx] / 100.0); // normalize to [0, 1]
    }
  }
  else {
    MGlobal::displayWarning("ComputeResults - Audio event volumes out of sync, so ignoring them");
  }
  
  // Loop through events
  for ( size_t idx = 0; idx < eventTimeContainer.length(); ++idx ) {
    const auto timeKey = static_cast<KeyframeTime>( eventTimeContainer[idx] );
    // Creat EventAction
    const auto eventStr = GetEnumStr( eventVals->second.attribute, static_cast<size_t>(eventValContainer[idx]) );
    // Probability
    auto prob = kInvalidAtt; // Will need to fix probabilty value after all keyframe events are queried
    const auto probMapIt = probMap.find( timeKey );
    if ( probMapIt != probMap.end() ) {
      prob = probMapIt->second;
    }
    // Volume
    auto volume = 1.0f;  // Default volume
    const auto volumeMapIt = volumeMap.find( timeKey );
    if ( volumeMapIt != volumeMap.end() ) {
      volume = volumeMapIt->second;
    }
    EventAction event( eventStr.asChar(), prob, volume );
    // Add event to Action
    auto eventIt = actionMap.find( timeKey );
    if ( eventIt == actionMap.end() ) {
      // New keyframe
      std::vector<EventAction> vec;
      vec.push_back( std::move(event) );
      actionMap.emplace( timeKey, std::move(vec) );
    }
    else {
      // Existing keyframe
      eventIt->second.push_back( std::move(event) );
    }
  }
  
  if ( MAYA_DEGUG_LOGS ) {
    MGlobal::displayInfo("ComputeResults - Results");
    for ( const auto& animEventMapIt : actionMap ) {
      for ( const auto& animEventIt : animEventMapIt.second ) {
        MGlobal::displayInfo("ComputeResults - time " + MString(std::to_string(animEventMapIt.first).c_str()) +
                             " event: " + MString(animEventIt.Description().c_str()) );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventActionCapture::ValidateComputedResults()
{
//  MGlobal::displayWarning("EventActionCapture::ValidateComputedResults!!");
  // Loop all keyframes
  for ( auto& anActionMapIt : actionMap ) {
    // Loop all events in actions
    bool validProb = true;
    for ( auto& anActionIt : anActionMapIt.second ) {
      if ( Anki::Util::IsNear( anActionIt.probability, kInvalidAtt ) ) {
        validProb = false;
        break;
      }
    }
    if ( !validProb ) {
      // Reset all event probabilities to be equally distributed
      const float prob = 1.0f / anActionMapIt.second.size();
      MGlobal::displayWarning("Audio keyframe at " + MString(std::to_string(anActionMapIt.first).c_str()) +
                              " has invalid event probabilities or none; they will be equally distributed to " +
                              MString(std::to_string(prob).c_str()));
      for ( auto& anActionIt : anActionMapIt.second ) {
        anActionIt.probability = prob;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateActionCapture::StateActionCapture()
{
  const std::string stateGroupIdAtt   = "stateGroupId";
  const std::string stateAtt          = "stateName";
  const std::string timeChangeQuery   = "timeChange";
  const std::string valueChangeQuery  = "valueChange";
  queryMap.emplace( "stateGroupIdTime", QueryDef( stateGroupIdAtt, timeChangeQuery, MIntArray() ) );
  queryMap.emplace( "stateGroupIdValue", QueryDef( stateGroupIdAtt, valueChangeQuery, MIntArray() ) );
  queryMap.emplace( "stateValue", QueryDef( stateAtt, valueChangeQuery, MIntArray() ) );
  enumListMap.emplace( stateGroupIdAtt, MStringArray() );
  enumListMap.emplace( stateAtt, MStringArray() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StateActionCapture::validQuery() {
  const auto& wwiseIdTimeLen = queryMap.find("stateGroupIdTime")->second.intContainer.length();
  return (wwiseIdTimeLen > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StateActionCapture::ComputeResults() {
  
  // Get event Timestamp
  const auto& stateGroupTimes = queryMap.find("stateGroupIdTime");
  const auto& stateGroupValue = queryMap.find("stateGroupIdValue");
  const auto& stateValue      = queryMap.find("stateValue");
  const auto stateGroupTimeCount = stateGroupTimes->second.intContainer.length();
  
  if ( (stateGroupTimeCount != stateGroupValue->second.intContainer.length()) &&
       (stateGroupTimeCount != stateValue->second.intContainer.length()) ) {
    MGlobal::displayError("ComputeResults - data arrays don't match length - stateGroupTime: " +
                          MString(std::to_string(stateGroupTimes->second.intContainer.length()).c_str()) +
                          " stateGroupValue: " + MString(std::to_string(stateGroupValue->second.intContainer.length()).c_str()) +
                          " stateValue: " + MString(std::to_string(stateValue->second.intContainer.length()).c_str()) );
    return;
  }
  
  for ( size_t idx = 0; idx < stateGroupTimeCount; ++idx ) {
    const auto timeKey = static_cast<KeyframeTime>( stateGroupTimes->second.intContainer[idx] );
    auto stateIt = actionMap.find( timeKey );
    StateAction action;
    action.stateGroup = GetEnumStr( stateGroupValue->second.attribute,
                                    static_cast<size_t>( stateGroupValue->second.intContainer[idx]) ).asChar();
    action.state = GetEnumStr( stateValue->second.attribute,
                               static_cast<size_t>( stateValue->second.intContainer[idx]) ).asChar();
    
    if ( stateIt == actionMap.end() ) {
      // First entry for time
      std::vector<StateAction> vec;
      vec.push_back( std::move(action) );
      actionMap.emplace( timeKey, std::move(vec) );
    }
    else {
      stateIt->second.push_back( std::move(action) );
    }
  }
  
  if ( MAYA_DEGUG_LOGS ) {
    MGlobal::displayInfo("ComputeResults - Results");
    for ( const auto& animStateMapIt : actionMap ) {
      for ( const auto& animStateIt : animStateMapIt.second ) {
        MGlobal::displayInfo("ComputeResults - time " + MString(std::to_string(animStateMapIt.first).c_str()) +
                             " state: " + MString(animStateIt.Description().c_str()) );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SwitchActionCapture::SwitchActionCapture()
{
  const std::string switchGroupIdAtt   = "switchGroupId";
  const std::string switchAtt          = "switchName";
  const std::string timeChangeQuery    = "timeChange";
  const std::string valueChangeQuery   = "valueChange";
  queryMap.emplace( "switchGroupIdTime", QueryDef( switchGroupIdAtt, timeChangeQuery, MIntArray() ) );
  queryMap.emplace( "switchGroupIdValue", QueryDef( switchGroupIdAtt, valueChangeQuery, MIntArray() ) );
  queryMap.emplace( "switchValue", QueryDef( switchAtt, valueChangeQuery, MIntArray() ) );
  enumListMap.emplace( switchGroupIdAtt, MStringArray() );
  enumListMap.emplace( switchAtt, MStringArray() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SwitchActionCapture::validQuery() {
  const auto& wwiseIdTimeLen = queryMap.find( "switchGroupIdTime" )->second.intContainer.length();
  return (wwiseIdTimeLen > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SwitchActionCapture::ComputeResults() {
  
  // Get event Timestamp
  const auto& switchGroupTimes = queryMap.find("switchGroupIdTime");
  const auto& switchGroupValue = queryMap.find("switchGroupIdValue");
  const auto& switchValue      = queryMap.find("switchValue");
  const auto switchGroupTimeCount = switchGroupTimes->second.intContainer.length();

  if ( (switchGroupTimeCount != switchGroupValue->second.intContainer.length()) &&
       (switchGroupTimeCount != switchValue->second.intContainer.length()) ) {
    MGlobal::displayInfo("ComputeResults - data arrays don't match length - switchGroupTime: " +
                         MString(std::to_string(switchGroupTimes->second.intContainer.length()).c_str()) +
                         " switchGroupValue: " + MString(std::to_string(switchGroupValue->second.intContainer.length()).c_str()) +
                         " switchValue: " + MString(std::to_string(switchValue->second.intContainer.length()).c_str()) );
    return;
  }
  
  for ( size_t idx = 0; idx < switchGroupTimeCount; ++idx ) {
    const auto timeKey = static_cast<KeyframeTime>( switchGroupTimes->second.intContainer[idx] );
    auto stateIt = actionMap.find(timeKey);
    SwitchAction action;
    action.switchGroup = GetEnumStr( switchGroupValue->second.attribute,
                                     static_cast<size_t>( switchGroupValue->second.intContainer[idx]) ).asChar();
    action.state = GetEnumStr( switchValue->second.attribute,
                               static_cast<size_t>( switchValue->second.intContainer[idx]) ).asChar();
    
    if ( stateIt == actionMap.end() ) {
      // First entry for time
      std::vector<SwitchAction> vec;
      vec.push_back( std::move(action) );
      actionMap.emplace( timeKey, std::move(vec) );
    }
    else {
      stateIt->second.push_back( std::move(action) );
    }
  }
  
  if ( MAYA_DEGUG_LOGS ) {
    MGlobal::displayInfo("ComputeResults - Results");
    for ( const auto& animStateMapIt : actionMap ) {
      for ( const auto& animStateIt : animStateMapIt.second ) {
        MGlobal::displayInfo("ComputeResults - time " + MString(std::to_string(animStateMapIt.first).c_str()) +
                             " switch: " + MString(animStateIt.Description().c_str()) );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ParameterActionCapture::ParameterActionCapture()
{
  const std::string parameterNameAtt      = "parameterName";
  const std::string parameterValAtt       = "value";
  const std::string parameterDurationAtt  = "timeMs";
  const std::string parameterCurveAtt     = "curveType";
  const std::string timeChangeQuery       = "timeChange";
  const std::string valueChangeQuery      = "valueChange";
  queryMap.emplace( "parameterNameTime", QueryDef( parameterNameAtt, timeChangeQuery, MIntArray() ) );
  queryMap.emplace( "parameterNameValue", QueryDef( parameterNameAtt, valueChangeQuery, MIntArray() ) );
  queryMap.emplace( "parameterValValue", QueryDef( parameterValAtt, valueChangeQuery, MDoubleArray() ) );
  queryMap.emplace( "parameterDurationValue", QueryDef( parameterDurationAtt, valueChangeQuery, MIntArray() ) );
  queryMap.emplace( "parameterCurveValue", QueryDef( parameterCurveAtt, valueChangeQuery, MIntArray() ) );
  enumListMap.emplace( parameterNameAtt, MStringArray() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ParameterActionCapture::validQuery() {
  const auto& wwiseIdTimeLen = queryMap.find("parameterNameTime")->second.intContainer.length();
  return (wwiseIdTimeLen > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ParameterActionCapture::ComputeResults() {
  // Get event Timestamp
  const auto& parameterNameTimes      = queryMap.find("parameterNameTime");
  const auto& parameterNameValue      = queryMap.find("parameterNameValue");
  const auto& parameterValValue       = queryMap.find("parameterValValue");
  const auto& parameterDurationValue  = queryMap.find("parameterDurationValue");
  const auto& parameterCurveValue     = queryMap.find("parameterCurveValue");
  const auto parameterTimeCount = parameterNameTimes->second.intContainer.length();
  
  if ( (parameterTimeCount != parameterNameValue->second.intContainer.length()) &&
       (parameterTimeCount != parameterValValue->second.intContainer.length()) ) {
    MGlobal::displayInfo("ComputeResults - data arrays don't match length - parameterGroupTime: " +
                         MString(std::to_string(parameterNameTimes->second.intContainer.length()).c_str()) +
                         " parameterGroupValue: " + MString(std::to_string(parameterNameValue->second.intContainer.length()).c_str()) +
                         " parameterValue: " + MString(std::to_string(parameterValValue->second.doubleContainer.length()).c_str()) );
    return;
  }
  
  for ( size_t idx = 0; idx < parameterTimeCount; ++idx ) {
    const auto timeKey = static_cast<KeyframeTime>( parameterNameTimes->second.intContainer[idx] );
    auto parameterIt = actionMap.find(timeKey);
    // Create Parameter Action
    ParameterAction action;
    // Name
    action.name = GetEnumStr( parameterNameValue->second.attribute,
                              static_cast<size_t>( parameterNameValue->second.intContainer[idx]) ).asChar();
    // Value
    action.value = static_cast<float>(parameterValValue->second.doubleContainer[idx]);
    
    // Duration
    action.duration_ms = static_cast<float>(parameterDurationValue->second.intContainer[idx]);
    
    // Interpolation curve
    // Align enum value from Maya with AudioCurveType
    auto audioCurve = static_cast<uint8_t>(parameterCurveValue->second.intContainer[idx]) - 1;
    if (audioCurve < 0) {
      // Ignore Maya's "No interpolation" state
      audioCurve = 0;
    }
    action.curveType = audioCurve;
    
    // Add to Action
    if ( parameterIt == actionMap.end() ) {
      // First entry for time
      std::vector<ParameterAction> vec;
      vec.push_back( std::move(action) );
      actionMap.emplace( timeKey, std::move(vec) );
    }
    else {
      parameterIt->second.push_back( std::move(action) );
    }
  }
  
  if ( MAYA_DEGUG_LOGS ) {
    MGlobal::displayInfo("ComputeResults - Results");
    for ( const auto& animStateMapIt : actionMap ) {
      for ( const auto& animStateIt : animStateMapIt.second ) {
        MGlobal::displayInfo("ComputeResults - time " + MString(std::to_string(animStateMapIt.first).c_str()) +
                             " parameter: " + MString(animStateIt.Description().c_str()) );
      }
    }
  }
}

