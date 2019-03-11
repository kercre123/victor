/**
 * File: alexa.cpp
 *
 * Author: ross
 * Created: Oct 16 2018
 *
 * Description: Wrapper for component that integrates the Alexa Voice Service (AVS) SDK. Alexa is an opt-in
 *              feature, so this class handles communication with the engine to opt in and out, and is
 *              otherwise a pimpl-style wrapper, although this class does a fair amount since the impl can be deleted
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#include "cozmoAnim/alexa/alexa.h"
#include "cozmoAnim/alexa/alexaImpl.h" // impl declaration

#include "cozmoAnim/animProcessMessages.h" // must come before clad includes........

#include "audioEngine/audioCallback.h"
#include "audioEngine/audioTypeTranslator.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"
#include "clad/types/alexaTypes.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/backpackLights/animBackpackLightComponent.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenTypes.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/showAudioStreamStateManager.h"
#include "util/fileUtils/fileUtils.h"
#include "util/console/consoleInterface.h"
#include "util/environment/locale.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"
#include "webServerProcess/src/webService.h"

#include <chrono>

namespace Anki {
namespace Vector {
  
// VIC-13319 remove
CONSOLE_VAR(bool, kAlexaEnabledInUK, "Alexa", true);
CONSOLE_VAR(bool, kAlexaEnabledInAU, "Alexa", true);
  
namespace {
  const std::string kAlexaPath = "alexa";
  const std::string kOptedInFile = "optedIn";
  const std::string kWasOptedInFile = "wasOptedIn";
  const std::string kWebVizModuleName = "alexa";
  #define LOG_CHANNEL "Alexa"
  
  const float kTimeUntilWakeWord_s = 3.0f;
  
  // If true, the wake word is enabled when signed out if the user has _ever_ been signed in. Saying "alexa"
  // triggers an error. If false, no wake work is running.
  const bool kPlayErrorIfSignedOut = false;
  
  const float kAlexaErrorTimeout_s = 15.0f; // max duration for error audio
  
  bool AlexaLocaleEnabled(const Util::Locale& locale)
  {
    if( locale.GetCountry() == Util::Locale::CountryISO2::US ) {
      return true;
    } else if( locale.GetCountry() == Util::Locale::CountryISO2::GB ) {
      return kAlexaEnabledInUK;
    } else if( locale.GetCountry() == Util::Locale::CountryISO2::AU ) {
      return kAlexaEnabledInAU;
    } else {
      return false;
    }
  }
}

CONSOLE_VAR(bool, kAllowAudioOnCharger, "Alexa", true);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngine::AudioEventId GetErrorAudioEvent( AlexaNetworkErrorType errorType )
{
  using namespace AudioEngine;
  using GenericEvent = AudioMetaData::GameEvent::GenericEvent;
  switch( errorType ) {
    case AlexaNetworkErrorType::NoInitialConnection:
      // "I'm having trouble connecting to the internet. For help, go to your device's companion app"
      return ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__Avs_System_Prompt_Error_Offline_Not_Connected_To_Internet );
    case AlexaNetworkErrorType::LostConnection:
      // "Sorry, your device lost its connection."
      return ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__Avs_System_Prompt_Error_Offline_Lost_Connection );
    case AlexaNetworkErrorType::HavingTroubleThinking:
      // "Sorry, I'm having trouble understanding right now. please try a little later"
      return ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__Avs_System_Prompt_Error_Offline_Not_Connected_To_Service_Else );
    case AlexaNetworkErrorType::AuthRevoked:
      // "Your device isnt registered. For help, go it its companion app"
      return ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__Avs_System_Prompt_Error_Offline_Not_Registered );
    case AlexaNetworkErrorType::NoError:
    default:
      return AudioEngine::kInvalidAudioEventId;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Alexa::Alexa()
: _authState{ AlexaAuthState::Uninitialized }
, _uxState{ AlexaUXState::Idle }
, _pendingUXState{ AlexaUXState::Idle }
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Alexa::~Alexa()
{
  if( _implDtorResult.valid() ) {
    _implDtorResult.wait();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::Init(const Anim::AnimContext* context)
{
  _context = context;

  // useful for testing shutting down alexa without losing your auth credentials
  auto deleteImpl = [this](ConsoleFunctionContextRef context ) {
    DeleteImpl();
  };
  _consoleFuncs.emplace_front( "DeleteImpl", std::move(deleteImpl), "Alexa", "" );

  // assume opted out. If there's a file indicating opted in, create the impl and try to authorize.
  // otherwise, wait for an engine msg saying to start authorization
  bool authenticatedLastBoot = DidAuthenticateLastBoot();
  _authenticatedEver = DidAuthenticateEver();

  if( authenticatedLastBoot ) {
    SetAlexaActive( true );
  } else {
    // make sure engine knows we're signed out
    SendAuthState();
    
    if( kPlayErrorIfSignedOut && _authenticatedEver ) {
      // alexa is not opted in, but the user was once authenticated. enable the wakeword so that
      // when they say the wake word, it plays "Your device isn't registered. For help, go to its companion app."
      // TODO: it might make sense to load the least sensitive model to avoid false positives
      SetSimpleState( AlexaSimpleState::Idle );
    }
  }
  
# if ALEXA_ACOUSTIC_TEST
  {
    // Prevent automatic reboots. (/run is ramdisk)
    // This works (for now) since vicAnim runs as root. A better place would be vic-init.sh, but
    // that doesnt have access to build flags yet.
    Util::FileUtils::TouchFile( "/run/inhibit_reboot" );
  }
# endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::Update()
{
  if( _implToBeDeleted != nullptr ) {
    // the impl destructor calls shutdown() on a bunch of components. During shutdown(), media players
    // join() on a processing loop that includes calls to thread::sleep_for, and other components
    // could also be doing work, so run the destructor on another thread.
    auto deleteImpl = [impl=std::move(_implToBeDeleted)](){
      delete impl;
      // won't provide any useful info if another impl was since created
      #if ANKI_DEV_CHEATS
        AlexaImpl::ConfirmShutdown();
      #endif
    };
    _implToBeDeleted = nullptr;
    ASSERT_NAMED( !_implDtorResult.valid(), "Alexa.Update.DtorThreadExists" );
    _implDtorResult = std::async( std::launch::async, std::move(deleteImpl) );
  }
  if( _implDtorResult.valid() && (_implDtorResult.wait_for(std::chrono::milliseconds{0}) == std::future_status::ready) ) {
    _implDtorResult = {};
  }
  
  if( _pendingLocale && HasInitializedImpl() ) {
    _impl->SetLocale( *_locale );
    _pendingLocale = false;
  }
  
  if( _impl != nullptr) {
    // should be called even if not initialized, because this drives the init process
    _impl->Update();
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  if( (_timeEnableWakeWord_s >= 0.0f) && (currTime_s >= _timeEnableWakeWord_s) ) {
    _timeEnableWakeWord_s = -1.0f;
    LOG_INFO("Alexa.Update.EnablingWakeWord", "Enabling the wakeword because of a delay in connecting");
    // enable the wakeword
    SetSimpleState( AlexaSimpleState::Idle );
  }
  
  if( (_timeToEndError_s >= 0.0f) && (currTime_s >= _timeToEndError_s) ) {
    // reset error flag, then set the ux state with whatever the impl most recently sent as the ux state
    _timeToEndError_s = -1.0f;
    LOG_INFO( "Alexa.Update.RestoreStateAfterError", "returning to UX state after error state is complete" );
    OnAlexaUXStateChanged( _pendingUXState );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetAlexaUsage(bool optedIn)
{
  LOG_INFO( "Alexa.SetAlexaUsage.Opting",
            "User requests to log %s",
            optedIn ? "in" : "out" );

  if( optedIn ) {
    // it's possible that an error condition was playing when we started the auth process. Don't let the callback from
    // that change the state.
    _timeToEndError_s = -1.0f;
  } else {
    // if we were in the process of authenticating alexa, cancel that now (does nothing if not loggin in).
    // need to do this before we reset _authStartedByUser
    CancelPendingAlexaAuth("OPT_OUT");
  }

  _authStartedByUser = optedIn;

  const bool loggingOut = !optedIn && HasImpl();
  if( loggingOut ) {
    // if the impl hasnt finished initializing, don't try calling any impl methods for safety. the user
    // would have to sign in and out quickly to do this, which isnt normally possible, and we nuke the
    // impl anyway
    if( _impl->IsInitialized() ) {
      // log out of amazon. this should delete persistent data, but we also nuke the folder just in case
      _impl->Logout();
    }
    // the sdk callback from this should call OnLogout, but just in case something went wrong, do it first here
    OnLogout();
  }
  
  // todo: if opting in, we might want to also nuke the alexa persistent folder in case some edge case didn't
  // properly delete it. For now, if there's an auth problem when opting in, the directory gets nuked, so
  // it will work the second time around. I'd rather keep it this way until we find the cases where it
  // doesn't get cleaned up initially.
  
  const bool deleteUserData = loggingOut;
  SetAlexaActive( optedIn,  deleteUserData );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetAlexaActive( bool active, bool deleteUserData )
{
  LOG_INFO( "Alexa.SetAlexaActive", "Internally active = %d", active );
  
  if( active && !HasImpl() ) {
    // wake word might be enabled because of a previous authentication. disable it while trying the actual auth process
    SetSimpleState( AlexaSimpleState::Disabled );
    // create impl
    CreateImpl();
  } else if( !active && HasImpl() ) {
    const auto simpleState = (_authenticatedEver && kPlayErrorIfSignedOut) ? AlexaSimpleState::Idle : AlexaSimpleState::Disabled;
    SetSimpleState( simpleState );
    DeleteImpl();
  }
  
  if( !active ) {
    // todo: we might want another option to delete the impl without deleting any data (this opt in file)
    // or logging out (in SetAlexaUsage()). That might be useful if we need to suddenly stop alexa during low
    // battery, etc

    // turn off our notification lights
    OnNotificationsChanged(false);
    
    DeleteOptInFile();
    if( deleteUserData ) {
      DeleteUserFiles();
    }
    // this is also set in other ways, but just to be sure
    SetAuthState( AlexaAuthState::Uninitialized );
    OnAlexaUXStateChanged( AlexaUXState::Idle );
    
    // If the user signs in again without rebooting, use the old locale
    if( _locale != nullptr ) {
      _pendingLocale = true;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::CancelPendingAlexaAuth(const std::string& reason)
{
  if( !_authStartedByUser ) {
    // various components in animprocess will call this method based on user actions like exiting pairing screen or
    // saying hey vector. We don't want this to cancel a pending authorization that started when the robot booted,
    // and instead only want it to cancel a user-initiated auth
    return;
  }

  LOG_INFO( "Alexa.CancelPendingAlexaAuth",
            "From auth state '%s', canceling for reason '%s'",
            EnumToString(_authState),
            reason.c_str() );
  
  switch( _authState ) {
    case AlexaAuthState::WaitingForCode:
    {

      DASMSG(sign_in_canceled,
             "alexa.user_sign_in_result",
             "Result of sign in attempt (this instance is for cancellations");
      DASMSG_SET(s1, "CANCEL", "result (CANCEL)");
      DASMSG_SET(s2, reason, "the reason this attempt was canceled");
      DASMSG_SEND();
    }
      // fall through
    case AlexaAuthState::RequestingAuth:
    {
      // if the robot is authorizing, cancel it. go through this method instead of SetAlexaActive so that any code face
      // is removed
      const bool errFlag = false;
      OnAlexaAuthChanged( AlexaAuthState::Uninitialized, "", "", errFlag );
    }
      break;
    case AlexaAuthState::Uninitialized:
    case AlexaAuthState::Authorized:
    {
      // the robot is either not using alexa, or is already authorized, so nothing to do here
    }
      break;
    case AlexaAuthState::Invalid:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::CreateImpl()
{
  if( !ANKI_VERIFY( _impl == nullptr, "Alexa.CreateImpl.Exists", "Alexa implementation already exists" ) ) {
    DeleteImpl();
  }
  
  _previousCode = "";
  
  // Set auth state that indicates that we are starting the auth process (RequestingAuth)
  // Setting this here ensures that authentication is never pending if
  // the state is Uninitialized. (the impl may also set this state through
  // a callback once it gets to that phase of initialization, but the current impl
  // design does not.)
  SetAuthState( AlexaAuthState::RequestingAuth );
  
  _impl = std::make_unique<AlexaImpl>();
  // set callbacks into this class (in case they occur during init)
  _impl->SetOnAlexaAuthStateChanged( std::bind( &Alexa::OnAlexaAuthChanged, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3,
                                                std::placeholders::_4) );
  _impl->SetOnAlexaUXStateChanged( std::bind( &Alexa::OnAlexaUXStateChanged, this, std::placeholders::_1 ) );
  _impl->SetOnLogout( std::bind( &Alexa::OnLogout, this) );
  _impl->SetOnNetworkError( std::bind( &Alexa::OnAlexaNetworkError, this, std::placeholders::_1 ) );
  _impl->SetOnNotificationsChanged( std::bind( &Alexa::OnNotificationsChanged, this, std::placeholders::_1 ) );
  // start an async initialization process
  auto initCallback = [this](bool success) {
    if( success ) {
      // initialization was successful, and the sdk is trying to connect at this point. If this is NOT a user-initiated
      // authentication attempt, turn on the wakeword by default after some amount of time has
      // elapsed. If they say the wake word before we can connect to avs, we play an error sound. If
      // there's a failure in auth, disable the wakeword. If auth changes in any way, clear the _timeEnableWakeWord_s.
      if( !_authStartedByUser && (_authState == AlexaAuthState::RequestingAuth) ) {
        _timeEnableWakeWord_s = kTimeUntilWakeWord_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
    } else {
      // initialization was unsuccessful. in case this means the alexa databases were corrupt, delete everything
      const bool deleteUserData = true;
      SetAlexaActive( false, deleteUserData );
    }
  };
  _impl->Init( _context, std::move(initCallback) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::DeleteImpl()
{
  {
    std::lock_guard<std::mutex> lg{ _implMutex };
    if( ANKI_VERIFY( _impl != nullptr, "Alexa.DeleteImpl.DoesntExist", "Alexa implementation doesnt exist" ) ) {
      // Prevent further callbacks from impl
      _impl->RemoveCallbacksForShutdown();
    }
    // Don't delete the impl just yet, since the current call to DeleteImpl could have originated from an impl callback
    ASSERT_NAMED( _implToBeDeleted == nullptr, "Alexa.DeleteImpl.Leak" );
    _implToBeDeleted = _impl.release();
  }
  SetAuthState( AlexaAuthState::Uninitialized );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Alexa::HasInitializedImpl() const
{
  if( _impl != nullptr ) {
    return _impl->IsInitialized(); // done loading SDK, but maybe not connected yet
  } else {
    return false;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnAlexaAuthChanged( AlexaAuthState state, const std::string& url, const std::string& code, bool errFlag )
{
  const auto oldState = _authState;
  bool codeExpired = false;

  LOG_INFO( "Alexa.OnAlexaAuthChanged", "from '%s' to '%s' url='%s' code='%s'",
            EnumToString(oldState),
            EnumToString(state),
            url.c_str(),
            code.c_str() );

  switch( state ) {
    case AlexaAuthState::Uninitialized:
    {
      _timeEnableWakeWord_s = -1.0f;
      const bool deleteUserData = errFlag;
      SetAlexaActive( false, deleteUserData ); // which also sets the state
    }
      break;
    case AlexaAuthState::RequestingAuth:
    {
      SetAuthState( state );
    }
      break;
    case AlexaAuthState::WaitingForCode:
    {
      _timeEnableWakeWord_s = -1.0f;
      if( !_authStartedByUser ) {
        // user didn't request to auth during this session, which means that an old token has expired. Logout
        const bool deleteUserData = true;
        SetAlexaActive( false, deleteUserData );
      } else if( !code.empty() && ((_previousCode == code) || _previousCode.empty()) ) {
        LOG_INFO( "Alexa.OnAlexaAuthChanged.FirstCode", "Received code '%s'", code.c_str() );
        _previousCode = code;
        SetAuthState( state, url, code );
      } else {
        LOG_DEBUG( "Alexa.OnAlexaAuthChanged.CodeRefresh", "Received another code (%s)", code.c_str());
        // this is not the first code, which means the first one expired. we don't have a good way of telling
        // the user that, so cancel authentication
        const bool deleteUserData = true;
        SetAlexaActive( false, deleteUserData );
        codeExpired = true;
      }
    }
      break;
    case AlexaAuthState::Authorized:
    {
      _timeEnableWakeWord_s = -1.0f;
      SetAuthState( state );
      TouchOptInFiles();
      _authenticatedEver = true;
      const auto simpleState = (_uxState == AlexaUXState::Idle) ? AlexaSimpleState::Idle : AlexaSimpleState::Active;
      SetSimpleState( simpleState );
    }
      break;
    case AlexaAuthState::Invalid:
      _timeEnableWakeWord_s = -1.0f;
      break;
  }
  
  
  // set face info screen. only show faces if the user opted in via app or voice command
  if( _authStartedByUser ) {
    // this uses state and oldState since one of the calls to SetAlexaActive above may
    // have changed _authState.
    if( errFlag ) {
      SetAlexaFace( ScreenName::AlexaPairingFailed );
    } else if( codeExpired ) {
      SetAlexaFace( ScreenName::AlexaPairingExpired );
    } else if( oldState != state ) {
      if( state == AlexaAuthState::WaitingForCode ) {
        SetAlexaFace( ScreenName::AlexaPairing, url, code );
      } else if( state == AlexaAuthState::Authorized ) {
        SetAlexaFace( ScreenName::AlexaPairingSuccess );
      } else if( state == AlexaAuthState::Uninitialized ) {
        // note: face info screen manager considers None as "not alexa," but might not switch to None
        SetAlexaFace( ScreenName::None );
      }
    }
  }
  
  if( _authState == AlexaAuthState::Authorized ) {
    // any future errors that only show when the user started the auth process no longer show once auth completes
    _authStartedByUser = false;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnLogout()
{
  bool deleteUserData = true;
  SetAlexaActive( false, deleteUserData );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnNotificationsChanged( bool hasNotification ) const
{
  // set backpack lights state
  auto* bplComp = _context->GetBackpackLightComponent();
  if( bplComp != nullptr ) {
    bplComp->SetAlexaNotification( hasNotification );
  }
  if( hasNotification ) {
    // play some animation for the user.
    // If animators want to avoid alexa "eye" pops, a queued notification would have to be separate
    // AlexaUXState. That would also mean more behavior work though...
    FaceInfoScreenManager::getInstance()->StartAlexaNotification();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetAuthState( AlexaAuthState state, const std::string& url, const std::string& code )
{
  _authExtra = url;
  if( _authState != state ) {
    _authState = state;
    SendAuthState();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetAlexaFace( ScreenName screenName, std::string url, const std::string& code ) const
{
  const bool showCodeFace = screenName == ScreenName::AlexaPairing;
  if( showCodeFace ) {
    // Wes says that we can bake in amazon.com/code
    url = "amazon.com/code";
    //  if( url.find("https://") != std::string::npos ) {
    //    url = url.substr(8);
    //  } else if( url.find("http://") != std::string::npos ) {
    //    url = url.substr(7);
    //  }
    // note: order of params is flipped since url may or may not be displayed
    FaceInfoScreenManager::getInstance()->EnableAlexaScreen( screenName, code, url );
  } else {
    FaceInfoScreenManager::getInstance()->EnableAlexaScreen( screenName, "", "" );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnAlexaUXStateChanged( AlexaUXState newState )
{
  // the impl should never send Error, because it doesn't internally consider that UX state, since the duration of that
  // state is determined by the duration of the audio clip we play. The audio clip must be played here instead of in
  // the impl because the latter may get destroyed when there is an authentication issue
  DEV_ASSERT( newState != AlexaUXState::Error, "Alexa.OnAlexaUXStateChanged.NoError" );
  
  _pendingUXState = newState;
  
  // wait for the error to finish before handling the new state. When the error finishes, this method
  // gets called again with _pendingUXState
  if( !IsErrorPlaying() ) {
    SetUXState( newState );
  }
  else {
    LOG_INFO( "Alexa.OnAlexaUXStateChanged.SetPendingAfterError",
              "An error is playing, so isntead of changing UX state now, set the pending state to '%s'",
              EnumToString(newState));
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetUXState( AlexaUXState newState )
{

  LOG_INFO( "Alexa.SetUXState", "new State '%s', old state was '%s'",
            EnumToString(newState),
            EnumToString(_uxState) );

  const auto oldState = _uxState;
  _uxState = newState;
  
  // send engine the UX state before starting any animations so that it knows what to do with animation end messages
  SendUXState();
  
  if( oldState != _uxState ) {
    // set backpack lights if streaming
    const bool listening = (_uxState == AlexaUXState::Listening);
    _context->GetBackpackLightComponent()->SetAlexaStreaming( listening );
    const bool speaking = ( _uxState == AlexaUXState::Speaking );
    _context->GetMicDataSystem()->GetSpeechRecognizerSystem()->SetAlexaSpeakingState( speaking );
  }
  
  if( _authState == AlexaAuthState::Authorized ) {
    const auto simpleState = (_uxState == AlexaUXState::Idle) ? AlexaSimpleState::Idle : AlexaSimpleState::Active;
    SetSimpleState( simpleState );
  }
  
  if( (oldState == AlexaUXState::Idle) && (_uxState != AlexaUXState::Idle) ) {
    // when transitioning out of idle, play a getin animation, if there is one.
    auto* showStreamStateManager = _context->GetShowAudioStreamStateManager();
    if( showStreamStateManager != nullptr ) {
      if( showStreamStateManager->HasValidAlexaUXResponse( _uxState ) ) {
        // play alexa get-in/audio requested by engine. switching out of mute plays an animation
        // that shan't be interrupted by the alexa get-in
        const bool skipGetIn = (_notifyType == NotifyType::ButtonFromMute);
        showStreamStateManager->StartAlexaResponse( _uxState, skipGetIn );
      }
    }
  }
  
  // Only play earcons when not frozen on charger (alexa acoustic test mode)
  if( !(_frozenOnCharger && _onCharger) || kAllowAudioOnCharger ) {
    using namespace AudioEngine;
    using GenericEvent = AudioMetaData::GameEvent::GenericEvent;
    // Play Audio Event for state change
    if (_uxState == AlexaUXState::Listening) {
      if ( (oldState == AlexaUXState::Idle) && (_notifyType != NotifyType::None) ) {
        // Alexa triggered by voice or button press
        PlayAudioEvent( ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__Sfx_Sml_Ui_Wakesound ) );
      }
      else if (oldState == AlexaUXState::Speaking) {
        // Play EarCon for follow up question
        PlayAudioEvent( ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__Sfx_Sml_Ui_Wakesound ) );
      }
      _notifyType = NotifyType::None;
    }
    else if( (oldState == AlexaUXState::Listening) && (_uxState == AlexaUXState::Thinking) ) {
      // Play when listening ends
      PlayAudioEvent( ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__Sfx_Sml_Ui_Endpointing ) );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnAlexaNetworkError( AlexaNetworkErrorType errorType )
{
  LOG_INFO( "Alexa.OnAlexaNetworkError.Type", "Error: %d", (int)errorType );
  // _uxState may be error if the user said the wake word while the error is playing. In that case, preserve the
  // old _pendingUXState, since _pendingUXState should never be Error
  if( _uxState != AlexaUXState::Error ) {
    _pendingUXState = _uxState;
  }
  SetUXState( AlexaUXState::Error );
  PlayErrorAudio( errorType );

  DASMSG(local_error_msg, "alexa.local_error", "A local (network) error response is being played");
  DASMSG_SET(s1,
             EnumToString(errorType),
             "type of the error (see AlexaNetworkErrorType in alexaTypes.clad)");
  DASMSG_SET(s2,
             EnumToString(_pendingUXState),
             "former UX state before the error happened (see alexaTypes.clad)");
  DASMSG_SEND();
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SendUXState()
{
  if( _engineLoaded ) {
    RobotInterface::AlexaUXChanged msg;
    msg.state = _uxState;
    RobotInterface::SendAnimToEngine( msg );
    LOG_INFO( "Alexa.SendUXState", "Sending state = %s", EnumToString(_uxState) );
  } else {
    // the ux state can change before engine init if a timer goes off, for example
    _pendingUXMsgs = true;
    LOG_INFO( "Alexa.SendUXState", "Pending state = %s", EnumToString(_uxState) );
  }
  SendStatesToWebViz();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::TouchOptInFiles() const
{
  const auto& fullPath = GetPersistentFolder();
  Util::FileUtils::TouchFile( fullPath + kOptedInFile );
  Util::FileUtils::TouchFile( fullPath + kWasOptedInFile );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Alexa::DidAuthenticateLastBoot() const
{
  const auto& fullPath = GetPersistentFolder() + kOptedInFile;
  const bool authenticatedLastBoot = !fullPath.empty() && Util::FileUtils::FileExists( fullPath );
  return authenticatedLastBoot;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Alexa::DidAuthenticateEver() const
{
  const auto& fullPath = GetPersistentFolder() + kWasOptedInFile;
  const bool authenticatedPreviously = !fullPath.empty() && Util::FileUtils::FileExists( fullPath );
  return authenticatedPreviously;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::DeleteOptInFile() const
{
  const auto& fullPath = GetPersistentFolder() + kOptedInFile;
  if( Util::FileUtils::FileExists( fullPath) ) {
    Util::FileUtils::DeleteFile( fullPath );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::DeleteUserFiles() const
{
  // delete all files except for kWasOptedInFile
  const std::string fullPath = GetPersistentFolder();
  const size_t pathLen = fullPath.size();
  if( Util::FileUtils::DirectoryExists( fullPath ) ) {
    const bool useFullPath = true; // ideally this would be false, but with recurse==true this param is forced to true
    const bool recurse = true;
    auto files = Util::FileUtils::FilesInDirectory( fullPath, useFullPath, nullptr, recurse);
    for( const auto& filename : files ) {
      if( filename.substr( pathLen ) != kWasOptedInFile ) {
        Util::FileUtils::DeleteFile( filename );
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string& Alexa::GetPersistentFolder() const
{
  static std::string fullPath;
  if( fullPath.empty() ) {
    const auto* dataPlatform = _context->GetDataPlatform();
    fullPath = dataPlatform->pathToResource( Util::Data::Scope::Persistent,
                                             kAlexaPath.c_str() );
    fullPath = Util::FileUtils::AddTrailingFileSeparator( fullPath );
  }
  return fullPath;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnEngineLoaded()
{
  _engineLoaded = true;
  if( _pendingAuthMsgs ) {
    _pendingAuthMsgs = false;
    SendAuthState();
  }
  if( _pendingUXMsgs ) {
    _pendingUXMsgs = false;
    SendUXState();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SendAuthState()
{
  if( _engineLoaded ) {
    RobotInterface::AlexaAuthChanged msg;
    msg.state = _authState;
    memcpy( msg.extra, _authExtra.c_str(), _authExtra.length() );
    msg.extra_length = _authExtra.length();

    RobotInterface::SendAnimToEngine( msg );
    LOG_INFO( "Alexa.SendAuthState", "Sending state = %s", EnumToString(_authState));
  } else {
    _pendingAuthMsgs = true;
    LOG_INFO( "Alexa.SendAuthState", "Pending state = %s", EnumToString(_authState));
  }
  SendStatesToWebViz();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SendStatesToWebViz() const
{
  if( _context != nullptr ) {
    auto* webService = _context->GetWebService();
    if( webService != nullptr && webService->IsWebVizClientSubscribed( kWebVizModuleName ) ) {
      Json::Value data;
      data["authState"] = EnumToString( _authState );
      data["uxState"] = EnumToString( _uxState );
      webService->SendToWebViz( kWebVizModuleName, data );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::AddMicrophoneSamples( const AudioUtil::AudioSample* const samples, size_t nSamples ) const
{
  // note: this can be called on another thread. guard access to _impl in case it is being deleted.
  // it might make more sense to move calls to this to the main thread as a precaution, sacrificing a copy
  std::lock_guard<std::mutex> lg{ _implMutex };
  if( HasInitializedImpl() ) {
    _impl->AddMicrophoneSamples( samples, nSamples );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Alexa::StopAlertIfActive()
{
  if( _impl != nullptr && _impl->IsAlertActive() ) {
    _impl->StopAlert();
    return true;
  } else {
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::NotifyOfTapToTalk( bool fromMute )
{
  if( HasInitializedImpl() ) {
    _notifyType = fromMute ? NotifyType::ButtonFromMute : NotifyType::Button;
    _impl->NotifyOfTapToTalk();
  } else {
    // sanity check
    ANKI_VERIFY( _impl != nullptr,
                 "Alexa.NotifyOfTapToTalk.Disabled",
                 "Tap-to-talk was issued when alexa was disabled" );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::NotifyOfWakeWord( uint64_t fromSampleIndex, uint64_t toSampleIndex )
{
  // wake word should not be enabled if impl is not initialized, but it doesn't hurt to check
  bool hasImpl = false;
  {
    std::lock_guard<std::mutex> lg{ _implMutex };
    hasImpl = HasImpl();
    if( hasImpl && _impl->IsInitialized() ) {
      _notifyType = NotifyType::Voice;
      _impl->NotifyOfWakeWord( fromSampleIndex, toSampleIndex );
    }
  }
  
  if( kPlayErrorIfSignedOut && !hasImpl && _authenticatedEver ) {
    OnAlexaNetworkError( AlexaNetworkErrorType::AuthRevoked );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::UpdateLocale( const Util::Locale& locale )
{
  if( !AlexaLocaleEnabled(locale) ) {
    return;
  }
  
  _locale.reset( new Util::Locale{locale} );
  if( HasInitializedImpl() ) {
    _impl->SetLocale( locale );
    _pendingLocale = false;
  } else {
    _pendingLocale = true;
  }
  // tell audio about the new locale for its baked in assets

  auto* audioController = _context->GetAudioController();
  if ( audioController != nullptr ) {
    using namespace AudioEngine;
    using namespace AudioMetaData;
    const auto stateGroup = ToAudioStateGroupId( GameState::StateGroupType::Robot_Alexa_Locale );
    GameState::Robot_Alexa_Locale newState = GameState::Robot_Alexa_Locale::En_Us;
    bool matched = true;
    switch( locale.GetCountry() ) {
      case Util::Locale::CountryISO2::US:
      case Util::Locale::CountryISO2::CA:
      {
        newState = GameState::Robot_Alexa_Locale::En_Us;
      }
        break;
      case Util::Locale::CountryISO2::GB:
      {
        newState = GameState::Robot_Alexa_Locale::En_Uk;
      }
        break;
      case Util::Locale::CountryISO2::AU:
      {
        newState = GameState::Robot_Alexa_Locale::En_Au;
      }
        break;
      default:
      {
        matched = false;
      }
        break;
    }
    if( matched ) {
      audioController->SetState( stateGroup, ToAudioStateId((AudioMetaData::GameState::GenericState)newState) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint64_t Alexa::GetMicrophoneSampleIndex() const
{
  std::lock_guard<std::mutex> lg{ _implMutex };
  if( HasInitializedImpl() ) {
    return _impl->GetMicrophoneTotalNumSamples();
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::PlayErrorAudio( AlexaNetworkErrorType errorType )
{
  LOG_INFO( "Alexa.PlayErrorAudio.Type", "Setting error flag %d", (int) errorType );
  DEV_ASSERT( errorType != AlexaNetworkErrorType::NoError, "Alexa.PlayErrorAudio.NotAnError" );
  
  if( !IsErrorPlaying() ) {
    using namespace AudioEngine;
    auto* callbackContext = new AudioCallbackContext();
    callbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
    callbackContext->SetExecuteAsync( false ); // run on main thread
    callbackContext->SetEventCallbackFunc( [this]( const AudioCallbackContext* thisContext,
                                                   const AudioCallbackInfo& callbackInfo )
    {
      if( IsErrorPlaying() ) {
        // reset error flag, then set the ux state with whatever the impl most recently sent as the ux state
        _timeToEndError_s = -1.0f;
        LOG_INFO( "Alexa.Callback.RestoreStateAfterError", "returning to UX state after error callback" );
        OnAlexaUXStateChanged( _pendingUXState );
      }
    });
    
    PlayAudioEvent( GetErrorAudioEvent( errorType ), callbackContext );
  }
  
  // extend timeout
  _timeToEndError_s = kAlexaErrorTimeout_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::PlayAudioEvent( AudioEngine::AudioEventId eventId, AudioEngine::AudioCallbackContext* callback ) const
{
  auto* audioController = _context->GetAudioController();
  if ( audioController != nullptr ) {
    using namespace AudioEngine;
    const auto gameObject = ToAudioGameObject( AudioMetaData::GameObjectType::Alexa );
    audioController->PostAudioEvent( eventId, gameObject, callback );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetSimpleState( AlexaSimpleState state ) const
{
  auto* mds = _context->GetMicDataSystem();
  if( mds != nullptr ) {
    mds->SetAlexaState( state );
  }
}
  
} // namespace Vector
} // namespace Anki
