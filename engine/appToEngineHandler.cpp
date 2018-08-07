/**
 * File: appToEngineHandler.cpp
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

#include "engine/appToEngineHandler.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/externalInterface/externalInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"
#include "webServerProcess/src/webService.h"

#include "json/json.h"

namespace Anki {
namespace Vector {
  
namespace {
  using Tag = ExternalInterface::MessageEngineToGameTag;
  
  // put here any engine to game tag that should be sent to the app
  std::vector<Tag> tags = {
    Tag::FaceEnrollmentCompleted,
    Tag::MeetVictorStarted,
    Tag::MeetVictorFaceScanStarted,
    Tag::MeetVictorFaceScanComplete,
    Tag::EnrolledNamesResponse
  };
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// convert from "which+robot%2Fhuman%3F" to "which robot/human?"
std::string UrlDecodeString(std::string str)
{
  Anki::Util::StringReplace( str, "+", " " );
  // no error checking! if your string has a % it better have a 2 digit hex code after it
  std::ostringstream escaped;
  while( true ) {
    std::size_t pos = str.find("%");
    if( pos != std::string::npos ) {
      // the part before the %
      escaped << str.substr(0, pos);
      // the 2 digit hex code after the %
      char byte = (char) (int)strtol(str.substr(pos+1,2).c_str(), 0, 16);
      escaped << byte;
      // the remainder of the string
      str = str.substr( pos+3, std::string::npos );
    } else {
      escaped << str;
      break;
    }
  }
  
  return escaped.str();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
// overloaded helpers to get types from json
bool SetFromJSONImpl(const Json::Value& json, std::string& val)
{
  if( json.isString() ) {
    val = json.asString();
    return true;
  } else {
    return false;
  }
}
bool SetFromJSONImpl(const Json::Value& json, bool& val)
{
  bool success = false;
  if( json.isBool() ) {
    val = json.asBool();
    success = true;
  } else if( json.isInt() ) {
    if( json.asInt() == 1 ) {
      val = true;
    } else if( json.asInt() == 0 ) {
      val = false;
    } else {
      val = (json.asInt() != 0);
      PRINT_NAMED_WARNING("AppToEngineHandler.SetFromJSONImpl.InvalidBool", "trying to set a bool with int [%d]", json.asInt());
    }
    success = true;
  } else if( json.isString() ) {
    if( json.asString() == "true" ) {
      val = true;
      success = true;
    } else if( json.asString() == "false" ) {
      val = false;
      success = true;
    }
  }
  return success;
}
  
bool SetFromJSONImpl(const Json::Value& json, int& val)
{
  bool success = false;
  if( json.isInt() ) {
    val = json.asInt();
    success = true;
  }
  return success;
}
  
#define A2E_SET_FROM_JSON(data, cladStruct, name) SetFromJSONImpl(data[#name], cladStruct.name)
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AppToEngineHandler::CreateCLAD( const Json::Value& data, ExternalInterface::MessageGameToEngine& msg ) const
{
  using namespace ExternalInterface;
  
  bool success = true;
  
  ExternalInterface::MessageGameToEngine tmpMsg;
  
  try { // jsoncpp throws exceptions we can catch to check for invalid params
  
    // put here deserializing of any messages you care about
    if( data["type"] == "SetFaceToEnroll" ) {
      SetFaceToEnroll sfte;
      success &= A2E_SET_FROM_JSON(data, sfte, name);
      success &= A2E_SET_FROM_JSON(data, sfte, observedID);
      success &= A2E_SET_FROM_JSON(data, sfte, saveID);
      success &= A2E_SET_FROM_JSON(data, sfte, saveToRobot);
      success &= A2E_SET_FROM_JSON(data, sfte, sayName);
      success &= A2E_SET_FROM_JSON(data, sfte, useMusic);
      tmpMsg.Set_SetFaceToEnroll(std::move(sfte));
    }
    else if( data["type"] == "AppIntent" ) {
      AppIntent appIntent;
      success &= A2E_SET_FROM_JSON(data, appIntent, intent);
      success &= A2E_SET_FROM_JSON(data, appIntent, param);
      tmpMsg.Set_AppIntent(std::move(appIntent));
    }
    else if( data["type"] == "EraseEnrolledFaceByID" ) {
      EraseEnrolledFaceByID erase;
      success &= A2E_SET_FROM_JSON(data, erase, faceID);
      tmpMsg.Set_EraseEnrolledFaceByID(std::move(erase));
    }
    else if( data["type"] == "UpdateEnrolledFaceByID" ) {
      UpdateEnrolledFaceByID update;
      success &= A2E_SET_FROM_JSON(data, update, faceID);
      success &= A2E_SET_FROM_JSON(data, update, newName);
      success &= A2E_SET_FROM_JSON(data, update, oldName);
      tmpMsg.Set_UpdateEnrolledFaceByID(std::move(update));
    }
    else if( data["type"] == "CancelFaceEnrollment" ) {
      CancelFaceEnrollment cancel;
      tmpMsg.Set_CancelFaceEnrollment(std::move(cancel));
    }
    else if( data["type"] == "RequestEnrolledNames" ) {
      RequestEnrolledNames request;
      tmpMsg.Set_RequestEnrolledNames(std::move(request));
    }
    else if( data["type"] == "EraseAllEnrolledFaces" ) {
      EraseAllEnrolledFaces eraseAll;
      tmpMsg.Set_EraseAllEnrolledFaces(std::move(eraseAll));
    }
    else {
      success = false;
    }
  } catch( std::exception ) {
    success = false;
  }
  
  if( success ) {
    msg = tmpMsg;
  }
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AppToEngineHandler::AppToEngineHandler()
{
  // reset the pending list
  GetPendingEngineToAppMessages();
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AppToEngineHandler::Init(WebService::WebService* ws, IExternalInterface* ei)
{
  _webService = ws;
  _externalInterface = ei;
  
  // message from engine to game ==> forward to app
  auto callback = std::bind(&AppToEngineHandler::HandleEngineToGameMessage, this, std::placeholders::_1);
  for( const auto& tag : tags ) {
    _signalHandles.push_back( _externalInterface->Subscribe(tag, callback) );
  }

  // webservice receives app data ==> parse it and send a game to engine message
  _signalHandles.push_back( _webService->OnAppToEngineOnData().ScopedSubscribe( [this](const std::string& params) {
    return ParseAppToEngineRequest( params );
  }) );
  
  // webservice says app requests engine to game messages ==> give it what it wants
  _signalHandles.push_back( _webService->OnAppToEngineRequestData().ScopedSubscribe( [this]() {
    return GetPendingEngineToAppMessages();
  }) );
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AppToEngineHandler::AssignBestGuess( Json::Value& data, const std::string& str ) const
{
  bool mightBeFloat = false;
  bool mightBeInt = !str.empty();
  
  for( const char& c : str ) {
    if( !std::isdigit(c) ) {
      mightBeInt = false;
      if( c != '.' ) {
        mightBeFloat = false;
        break;
      }
    }
    if( mightBeFloat && (c == '.') ) { // there can only be one .
      mightBeFloat = false;
      break;
    } else if( c == '.' ) {
      mightBeFloat = true;
    }
    if( !mightBeInt && !mightBeFloat ) {
      break;
    }
  }
  
  bool successfulCast = false;
  if( mightBeFloat && mightBeInt ) {
    try {
      float f = std::stof( str );
      data = f;
      successfulCast = true;
    } catch ( std::exception ) {
      
    }
  } else if( mightBeInt ) {
    try {
      long long l = std::stoll( str );
      data = l;
      successfulCast = true;
    } catch ( std::exception ) {
      
    }
  }
    
  if( !successfulCast ) {
    data = str;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string AppToEngineHandler::ParseAppToEngineRequest( const std::string& params ) const
{
  std::string ret = "fail";
  if( !ANKI_DEV_CHEATS ) { // just in case this is somehow called in shipping
    return ret;
  }
  
  if( _externalInterface == nullptr ) {
    return ret;
  }
  
  if( params.substr(0,5) == "type=" ) {
    
    std::vector<std::string> elems;
    if( params.find('&') == std::string::npos ) {
      elems.push_back( params );
    } else {
      std::stringstream ss(params);
      std::string item;
      while( std::getline(ss, item, '&') ) {
        elems.push_back( std::move(item) );
      }
    }
    
    // elems should now be of the form "arg1=val1"
    
    Json::Value data;
    bool foundType = false;
    for( const auto& elem : elems ) {
      size_t pos = elem.find('=');
      if( pos != std::string::npos ) {
        std::string arg = UrlDecodeString(elem.substr(0, pos));
        std::string val = UrlDecodeString(elem.substr(pos+1));
        foundType |= (arg == "type");
        AssignBestGuess( data[arg], val );
      }
    }
    DEV_ASSERT( foundType, "request did not contain a type");
      
    ExternalInterface::MessageGameToEngine cladMsg;
    
    if( CreateCLAD( data, cladMsg ) ) {
      ret = "success";
      _externalInterface->Broadcast( cladMsg );
    } else {
      PRINT_NAMED_WARNING("WebService.TempAppToRobot.Invalid","Could not create clad union from data '%s'", params.c_str());
    }
    
  }
  
  return ret;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string AppToEngineHandler::GetPendingEngineToAppMessages()
{
  if( !ANKI_DEV_CHEATS ) { // just in case this is somehow called in shipping
    return "";
  }
  _pendingEngineToApp << "]";
  std::string ret = _pendingEngineToApp.str();
  
  _pendingEngineToApp.str(""); // clear
  _pendingCount = 0;
  _pendingEngineToApp << "[";
  
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AppToEngineHandler::HandleEngineToGameMessage( const AnkiEvent<ExternalInterface::MessageEngineToGame>& event )
{
  
  Json::Value json;
  const auto& data = event.GetData();
  auto tag = data.GetTag();
  
  json["type"] = MessageEngineToGameTagToString(tag);
  
  // manually serialize since these are "message"s not "structure"s in CLAD
  #define A2E_SET_JSON(json, data, name) json[#name] = data.name
  
  if( tag == Tag::FaceEnrollmentCompleted ) {
    auto& msg = data.Get_FaceEnrollmentCompleted();
    A2E_SET_JSON(json, msg, faceID);
    A2E_SET_JSON(json, msg, name);
    json["result"] = FaceEnrollmentResultToString(msg.result);
  }
  else if( tag == Tag::EnrolledNamesResponse ) {
    auto& msg = data.Get_EnrolledNamesResponse();
    auto& jsonList = json["faces"];
    for( const auto& faceEntry : msg.faces ) {
      Json::Value jsonEntry;
      A2E_SET_JSON(jsonEntry, faceEntry, secondsSinceFirstEnrolled);
      A2E_SET_JSON(jsonEntry, faceEntry, secondsSinceLastUpdated);
      A2E_SET_JSON(jsonEntry, faceEntry, secondsSinceLastSeen);
      A2E_SET_JSON(jsonEntry, faceEntry, faceID);
      A2E_SET_JSON(jsonEntry, faceEntry, name);
      jsonList.append( jsonEntry );
    }
  }
  else if( tag == Tag::MeetVictorStarted ) {
    auto& msg = data.Get_MeetVictorStarted();
    json["name"] = msg.name;
  }
  else if( tag == Tag::MeetVictorFaceScanStarted ) {
    // do nothing. this is just here so people know this is handled
  }
  else if( tag == Tag::MeetVictorFaceScanComplete ) {
    // do nothing. this is just here so people know this is handled
  }
  
  
  if( _pendingCount > 0 ) {
    _pendingEngineToApp << ",";
  }
  _pendingEngineToApp << json;
  ++_pendingCount;
}
  
} // namespace Vector
} // namespace Anki
