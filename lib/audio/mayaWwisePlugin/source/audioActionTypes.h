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


#ifndef __Anki__Maya_AudioActionTypes_H__
#define __Anki__Maya_AudioActionTypes_H__

#include "mayaIncludes.h"
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define MAYA_DEGUG_LOGS 0

struct AudioKeyframe;

using KeyframeTime = uint;
using AudioKeyFrameMap = std::unordered_map< KeyframeTime, AudioKeyframe >;

// ---------------------------------------------------------------------------------------------------------------------
// Action Types
// Set of structs to store audio action data
// ---------------------------------------------------------------------------------------------------------------------
struct EventAction {
  std::string name;
  uint32_t    eventId = 0;
  float       probability = 0;
  float       volume = 0;
  EventAction( std::string name, float probability, float volume );
  const std::string Description() const;
};

struct StateAction {
  std::string stateGroup;
  std::string state;
  const std::string Description() const;
};

struct SwitchAction {
  std::string switchGroup;
  std::string state;
  const std::string Description() const;
};

struct ParameterAction {
  std::string name;
  float       value  = 0;
  uint32_t    duration_ms = 0;
  uint8_t     curveType = 0;
  const std::string Description() const;
};


// ---------------------------------------------------------------------------------------------------------------------
// Audio Keyframe
// A single keyframe of audio actions
// ---------------------------------------------------------------------------------------------------------------------
struct AudioKeyframe {
  
  AudioKeyframe() {};
  
  std::vector<EventAction> events;
  std::vector<StateAction> states;
  std::vector<SwitchAction> switches;
  std::vector<ParameterAction> parameters;
  
  const std::string Description() const;
  
  template<typename ActionType>
  std::vector<ActionType>& GetTypeVector();

  // Get Event with using probability
  // Reuurn nullptr if no event was chosen
  const EventAction* GetAudioEventToPlay() const;
};


// ---------------------------------------------------------------------------------------------------------------------
// Query Definition
// Define a maya query to be performed by AudioActionCapture
// ---------------------------------------------------------------------------------------------------------------------
// This is just a slice, need to get more than one layer
struct QueryDef {
  
  QueryDef( const std::string attribute, const std::string queryType, MDoubleArray&& container );
  QueryDef( const std::string attribute, const std::string queryType, MIntArray&& container );
  QueryDef( const std::string attribute, const std::string queryType, MStringArray&& container );
  
  ~QueryDef() {}
  QueryDef( const QueryDef& other );
  QueryDef& operator=( const QueryDef& other );
  
  // Perform Maya Query
  void PerformQuery( uint nodeIdx );
  // Reset data for next Query
  void Clear();
  
  enum class ContainerType {
    Double,
    Int,
    String
  };
  
  std::string attribute;
  std::string queryType;
  
  ContainerType type;
  union {
    MDoubleArray doubleContainer;
    MIntArray    intContainer;
    MStringArray stringContainer;
  };
  
  MStatus executeStatus;
};


// ---------------------------------------------------------------------------------------------------------------------
// Action Types
// Base class to capture maya audio keyframe data
// ---------------------------------------------------------------------------------------------------------------------
template <class ActionType>
class AudioActionCapture {
public:
  
  virtual ~AudioActionCapture() {};
  
  // Clear all data
  void Reset();
  // Perform all queries in Action Capture
  void QueryMaya();
  // Add Action Capture results to Audio Keyframe
  void AddActionsToMap( AudioKeyFrameMap& inOut_keyframeMap );
  
protected:
  
  // Get all necessary enum lists
  void UpdateEnumLists();
  // Get Enum name string
  const MString GetEnumStr( const std::string& enumListName, size_t idxVal );
  // Compute results from each query cycle
  virtual void ComputeResults() = 0;
  virtual void ValidateComputedResults() {};
  // Return true if current query is valid
  virtual bool validQuery() = 0;
  
  // Action Queries
  std::map<std::string, QueryDef> queryMap;
  // Key: Enum name
  // Val: Enum value list
  std::map<std::string, MStringArray> enumListMap;
  // Key: Time_ms
  // Val: Vector of Action Type
  std::map<KeyframeTime, std::vector<ActionType>> actionMap;
};  // AudioActionCapture

// Capture Event Group actions
class EventActionCapture : public AudioActionCapture<EventAction> {
public:
  EventActionCapture();
  bool validQuery() override;
  void ComputeResults() override;
  virtual void ValidateComputedResults() override;
}; // EventActionCapture

// Capture State actions
class StateActionCapture : public AudioActionCapture<StateAction> {
public:
  StateActionCapture();
  bool validQuery() override;
  void ComputeResults() override;
}; // StateActionCapture

// Capture Switch actions
class SwitchActionCapture : public AudioActionCapture<SwitchAction> {
public:
  SwitchActionCapture();
  bool validQuery() override;
  void ComputeResults() override;
}; // SwitchActionCapture

// Capture Parameter actions
class ParameterActionCapture : public AudioActionCapture<ParameterAction> {
public:
  ParameterActionCapture();
  bool validQuery() override;
  void ComputeResults() override;
}; // ParameterActionCapture


// ---------------------------------------------------------------------------------------------------------------------
// Template Imp
// ---------------------------------------------------------------------------------------------------------------------
template <class ActionType>
void AudioActionCapture<ActionType>::Reset() {
  actionMap.clear();
  for ( auto& aQuery : queryMap ) {
    aQuery.second.Clear();
  }
  for ( auto& anEnumList: enumListMap ) {
    anEnumList.second.clear();
  }
}

template <class ActionType>
void AudioActionCapture<ActionType>::QueryMaya()
{
  static const uint kMaxLoopCount = 100;
  MStatus querySliceStatus = MStatus::kSuccess;
  bool allAttributesSucces = true;
  uint idx = 0;
  std::string attributeIdx;
  // Update Enums
  UpdateEnumLists();
  // Query for all ActionType "actions"
  do {
    // Reset for next Query
    for ( auto& queryMapIt : queryMap ) {
      queryMapIt.second.Clear();
    }
    querySliceStatus = MStatus::kSuccess;
    
    // Set attribute Idx string
    if ( idx > 0 ) {
      // Don't add index to first idx
      attributeIdx = std::to_string( idx + 1 );
    }
    
    // Perform Queries
    for ( auto& queryMapIt : queryMap ) {
      auto& aQuery = queryMapIt.second;
      aQuery.PerformQuery( idx );
      if ( aQuery.executeStatus != MStatus::kSuccess ) {
        MGlobal::displayError("Mel Commands Status: " +  aQuery.executeStatus.errorString());
        allAttributesSucces = false;
      }
    }
    
    // Get Results
    ComputeResults();
    // Get while loop status
    allAttributesSucces &= validQuery();
    ++idx;
    // Make sure we don't have a run away while loop
    if ( idx == kMaxLoopCount ) {
      MGlobal::displayError("AudioActionCapture.QueryMaya() - Too many actions or run away while loop!");
      break;
    }
  } while ( allAttributesSucces );
  // Final validation for all queried actions
  ValidateComputedResults();
}

template <class ActionType>
void AudioActionCapture<ActionType>::AddActionsToMap( AudioKeyFrameMap& inOut_keyframeMap ) {
  // Loop through actions
  for ( const auto& actionMapIt : actionMap ) {
    // Check if there is a key frame with that time yet
    const auto& keyframeMapIt = inOut_keyframeMap.find( actionMapIt.first );
    if ( keyframeMapIt == inOut_keyframeMap.end() ) {
      // Add Keyframe
      AudioKeyframe keyframe;
      auto& typeVec = keyframe.GetTypeVector<ActionType>();
      typeVec = actionMapIt.second;
      inOut_keyframeMap.emplace( actionMapIt.first, std::move(keyframe) );
    }
    else {
      // Add events to key frame that already exist
      AudioKeyframe& keyframe = keyframeMapIt->second;
      auto& typeVec = keyframe.GetTypeVector<ActionType>();
      typeVec = actionMapIt.second; // Copy keyframe events
    }
  }
  
  if ( MAYA_DEGUG_LOGS ) {
    for ( auto& keyframeMapIt : inOut_keyframeMap ) {
      MString timeStr(std::to_string( keyframeMapIt.first ).c_str());
      for ( const auto& action : keyframeMapIt.second.GetTypeVector<ActionType>() ) {
        MGlobal::displayInfo("EventActionCapture.AddActionsToMap - time: " + timeStr + " - " +
                             MString(action.Description().c_str()));
      }
    }
  }
}

template <class ActionType>
void AudioActionCapture<ActionType>::UpdateEnumLists()
{
  // Qurery all enum list
  for ( auto& anEventListMapIt : enumListMap ) {
    MString enumListName( anEventListMapIt.first.c_str() );
    // Check if attribute type is used
    MString existCmd("objExists \"x:AnkiAudioNode." + enumListName +"\"");
    int result;
    MGlobal::executeCommand( existCmd, result );
    // If attribute doesn't exist move on to the next
    if (result != 1) {
      continue;
    }
    
    // Have attribute type query for nodes
    MString enumCmd("attributeQuery -node \"x:AnkiAudioNode\" -listEnum \"" + enumListName + "\"");
    MStringArray strArray;
    MStatus enumStatus = MGlobal::executeCommand( enumCmd, strArray );
    
    if ( enumStatus == MStatus::kSuccess && strArray.length() > 0 ) {
      if ( MAYA_DEGUG_LOGS ) {
        MGlobal::displayInfo("enum query '" + enumListName + "' passed");
        for (size_t idx = 0; idx < anEventListMapIt.second.length(); ++idx) {
          MGlobal::displayInfo("StateActionCapture::UpdateEnums - split - " + anEventListMapIt.second[idx]);
        }
      }
      enumStatus = strArray[0].split( ':', anEventListMapIt.second );
    }
    else {
      enumStatus = MStatus::kFailure;
      MGlobal::displayError("enum query '" + enumListName + "' failed");
    }
  }
}

template <class ActionType>
const MString AudioActionCapture<ActionType>::GetEnumStr( const std::string& enumListName, size_t idxVal )
{
  const auto& enumListMapIt = enumListMap.find( enumListName );
  if ( enumListMapIt == enumListMap.end() ) {
    MGlobal::displayError("AudioActionCapture.GetEnumStr - eventList '" + MString(enumListName.c_str()) +
                          "' does't exists");
    return MString();
  }
  if ( idxVal >= enumListMapIt->second.length() ) {
    MGlobal::displayError("AudioActionCapture.GetEnumStr - eventList '" + MString(enumListName.c_str()) +
                          "' Idx " + MString(std::to_string(idxVal).c_str()) + "does't exists");
    return MString();
  }
  
  return enumListMapIt->second[idxVal];
}

#endif /* __Anki__Maya_AudioActionTypes_H__ */
