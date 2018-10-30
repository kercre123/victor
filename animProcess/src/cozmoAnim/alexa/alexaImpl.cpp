/**
 * File: alexaImpl.cpp
 *
 * Author: ross
 * Created: Oct 16 2018
 *
 * Description: Component that integrates with the Alexa Voice Service (AVS) SDK
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#include "cozmoAnim/alexa/alexaImpl.h"

#include "clad/types/alexaAuthState.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "cozmoAnim/animContext.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {
  
/***************************
 FIRST VERSION IN MASTER JUST FAKES AUTHORIZATION STATES. Ignore this cpp file. Review the rest
******************************/
  
namespace {
  #define CONSOLE_GROUP "Alexa"
  CONSOLE_VAR_ENUM(uint8_t, kAlexaAuthenticationPath, CONSOLE_GROUP, 3, "InitialSuccess,Timeout,CodeExpires,SuccessWithoutCode");
  
  CONSOLE_VAR_ENUM(uint8_t, kAlexaAuthState, CONSOLE_GROUP, 1,
                   "Invalid,Uninitialized,RequestingAuth,WaitingForCode,Authorized");
  
  // hacky static stuff to not contaminate the header
  static auto sOldState = kAlexaAuthState;
  float sNextTime1 = 0.0f;
  float sNextTime2 = 0.0f;
  float sNextTime3 = 0.0f;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaImpl::AlexaImpl()
{
  sNextTime1 = 0.0f;
  sNextTime2 = 0.0f;
  sNextTime3 = 0.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaImpl::~AlexaImpl()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaImpl::Init(const AnimContext* context)
{
  _context = context;
  
  const auto* dataPlatform = _context->GetDataPlatform();
  _alexaFolder = dataPlatform->pathToResource( Util::Data::Scope::Persistent, "alexa" );
  
  if( !_alexaFolder.empty() && Util::FileUtils::DirectoryDoesNotExist( _alexaFolder ) ) {
    Util::FileUtils::CreateDirectory( _alexaFolder );
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::Update()
{
  if( sOldState != kAlexaAuthState ) {
    sOldState = kAlexaAuthState;
    
    const auto enumState = static_cast<AlexaAuthState>( kAlexaAuthState );
    if( _onAlexaAuthStateChanged != nullptr ) {
      if( enumState == AlexaAuthState::WaitingForCode ) {
        _onAlexaAuthStateChanged( enumState, "http://amazon.com/us/code", "consolecode" );
      } else {
        _onAlexaAuthStateChanged( enumState, "", "" );
      }
    }
    sNextTime1 = -1.0f;
    sNextTime2 = -1.0f;
    sNextTime3 = -1.0f;
    PRINT_NAMED_WARNING("Alexa","Resetting update");
  }
  
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if( _onAlexaAuthStateChanged != nullptr ) {
    if( (sNextTime1>0.0f) && currTime_s >= sNextTime1 ) {
      PRINT_NAMED_WARNING("Alexa"," time point 1: %d", kAlexaAuthenticationPath );
      sNextTime1 = -1.0f; // done
      if( kAlexaAuthenticationPath == 0 ) { // success
        _onAlexaAuthStateChanged( AlexaAuthState::RequestingAuth, "", "" );
      } else if( kAlexaAuthenticationPath == 1 ) { // timeout
        _onAlexaAuthStateChanged( AlexaAuthState::RequestingAuth, "", "" );
      } else if( kAlexaAuthenticationPath == 2 ) { // repeated code
        _onAlexaAuthStateChanged( AlexaAuthState::RequestingAuth, "", "" );
      } else if( kAlexaAuthenticationPath == 3 ) { // success without code
        _onAlexaAuthStateChanged( AlexaAuthState::RequestingAuth, "", "" );
      }
    }
    if( (sNextTime2>0.0f) && currTime_s >= sNextTime2 ) {
      PRINT_NAMED_WARNING("Alexa"," time point 2: %d", kAlexaAuthenticationPath );
      sNextTime2 = -1.0f; // done
      if( kAlexaAuthenticationPath == 0 ) { // success
        _onAlexaAuthStateChanged( AlexaAuthState::WaitingForCode, "http://amazon.com/us/code", "ABCDEF" );
      } else if( kAlexaAuthenticationPath == 1 ) { // timeout
        _onAlexaAuthStateChanged( AlexaAuthState::Uninitialized, "", "" );
      } else if( kAlexaAuthenticationPath == 2 ) { // repeated code
        _onAlexaAuthStateChanged( AlexaAuthState::WaitingForCode, "http://amazon.com/us/code", "ABCDEF" );
      } else if( kAlexaAuthenticationPath == 3 ) { // success without code
        _onAlexaAuthStateChanged( AlexaAuthState::Authorized, "", "");
        sNextTime3 = -1.0f;
      }
    }
    if( (sNextTime3>0.0f) && currTime_s >= sNextTime3 ) {
      PRINT_NAMED_WARNING("Alexa"," time point 3: %d", kAlexaAuthenticationPath );
      sNextTime3 = -1.0f; // done
      if( kAlexaAuthenticationPath == 0 ) { // success
        _onAlexaAuthStateChanged( AlexaAuthState::Authorized, "", "");
      } else if( kAlexaAuthenticationPath == 1 ) { // timeout
        // nada
      } else if( kAlexaAuthenticationPath == 2 ) { // repeated code
        _onAlexaAuthStateChanged( AlexaAuthState::WaitingForCode, "http://amazon.com/us/code", "GHIJKL" );
      }
    }
  }
  if( sNextTime1 == 0.0f ) {
    PRINT_NAMED_WARNING("Alexa","Setting based on %f", currTime_s);
    sNextTime1 = 10.0f + currTime_s;
    sNextTime2 = 20.0f + currTime_s;
    sNextTime3 = 30.0f + currTime_s;
  }
}

} // namespace Vector
} // namespace Anki
