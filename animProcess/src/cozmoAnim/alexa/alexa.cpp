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

#include "audioEngine/audioCallback.h"
#include "audioEngine/audioTypeTranslator.h"
#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/backpackLights/animBackpackLightComponent.h"
#include "clad/types/alexaTypes.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenTypes.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/showAudioStreamStateManager.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "webServerProcess/src/webService.h"



namespace Anki {
namespace Vector {
  
namespace {
  const std::string kAlexaPath = "alexa";
  const std::string kOptedInFile = "optedIn";
  const std::string kWebVizModuleName = "alexa";
  #define LOG_CHANNEL "Alexa"
  
  const float kTimeUntilWakeWord_s = 3.0f;
  
  
  const float kAlexaErrorTimeout_s = 15.0f; // max duration for error audio
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngine::AudioEventId GetErrorAudioEvent( AlexaNetworkErrorType errorType )
{
  using namespace AudioEngine;
  using GenericEvent = AudioMetaData::GameEvent::GenericEvent;
  switch( errorType ) {
    case AlexaNetworkErrorType::NoInitialConnection:
      // "I'm having trouble connecting to the internet. For help, go to your device's companion app"
      return ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__En_Us_Avs_System_Prompt_Error_Offline_Not_Connected_To_Internet );
    case AlexaNetworkErrorType::LostConnection:
      // "Sorry, your device lost its connection."
      return ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__En_Us_Avs_System_Prompt_Error_Offline_Lost_Connection );
    case AlexaNetworkErrorType::HavingTroubleThinking:
      // "Sorry, I'm having trouble understanding right now. please try a little later"
      return ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__En_Us_Avs_System_Prompt_Error_Offline_Not_Connected_To_Service_Else );
    case AlexaNetworkErrorType::AuthRevoked:
      // "Your device isnt registered. For help, go it its companion app"
      return ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__En_Us_Avs_System_Prompt_Error_Offline_Not_Registered );
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
// is defined here since AlexaImpl is not defined in the header
Alexa::~Alexa() = default;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::Init(const AnimContext* context)
{
  _context = context;
  
  // assume opted out. If there's a file indicating opted in, create the impl and try to authorize.
  // otherwise, wait for an engine msg saying to start authorization
  bool previouslyAuthenticated = DidAuthenticatePreviously();
  SetAlexaActive( previouslyAuthenticated );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::Update()
{
  if( _impl != nullptr) {
    _impl->Update();
  }
  
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if( (_timeEnableWakeWord_s >= 0.0f) && (currTime_s >= _timeEnableWakeWord_s) ) {
    _timeEnableWakeWord_s = -1.0f;
    // TODO (VIC-11517): downgrade. for now this is useful in webots
    LOG_WARNING("Alexa.Update.EnablingWakeWord", "Enabling the wakeword because of a delay in connecting");
    // enable the wakeword
    auto* mds = _context->GetMicDataSystem();
    if( mds != nullptr ) {
      mds->SetAlexaState( AlexaSimpleState::Idle );
    }
  }
  
  if( (_timeToEndError_s >= 0.0f) && (currTime_s >= _timeToEndError_s) ) {
    // reset error flag, then set the ux state with whatever the impl most recently sent as the ux state
    _timeToEndError_s = -1.0f;
    OnAlexaUXStateChanged( _pendingUXState );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetAlexaUsage(bool optedIn)
{
  LOG_INFO( "Alexa.SetAlexaUsage.Opting",
            "User requests to log %s",
            optedIn ? "in" : "out" );
  
  _authStartedByUser = optedIn;
  const bool loggingOut = !optedIn && HasImpl();
  if( loggingOut ) {
    // log out of amazon. this should delete persistent data, but we also nuke the folder just in case
    _impl->Logout();
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
    CreateImpl();
  } else if( !active && HasImpl() ) {
    DeleteImpl();
    
    auto* mds = _context->GetMicDataSystem();
    if( mds != nullptr ) {
      mds->SetAlexaState( AlexaSimpleState::Disabled );
    }
  }
  
  if( !active ) {
    // todo: we might want another option to delete the impl without deleting any data (this opt in file)
    // or logging out (in SetAlexaUsage()). That might be useful if we need to suddenly stop alexa during low
    // battery, etc
    
    DeleteOptInFile();
    if( deleteUserData ) {
      DeleteUserFolder();
    }
    // this is also set in other ways, but just to be sure
    SetAuthState( AlexaAuthState::Uninitialized );
    OnAlexaUXStateChanged( AlexaUXState::Idle );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::CancelPendingAlexaAuth()
{
  switch( _authState ) {
    case AlexaAuthState::RequestingAuth:
    case AlexaAuthState::WaitingForCode:
    {
      // if the robot is authorizing, cancel it
      SetAlexaActive( false );
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
  const bool success = _impl->Init( _context );
  if( success ) {
    // initialization was successful.
    // The sdk is likely trying to connect at this point. If this is NOT a user-initiated
    // authentication attempt, turn on the wakeword by default after some amount of time has
    // elapsed. If they say the wake word before we can connect to avs, we play an error sound. If
    // there's a failure in auth, disable the wakeword. If auth changes in any way, clear the _timeEnableWakeWord_s.
    if( !_authStartedByUser && (_authState == AlexaAuthState::RequestingAuth) ) {
      _timeEnableWakeWord_s = kTimeUntilWakeWord_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    }
  } else {
    // initialization was unsuccessful
    SetAlexaActive( false );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::DeleteImpl()
{
  std::lock_guard<std::mutex> lg{ _implMutex };
  ANKI_VERIFY( _impl != nullptr, "Alexa.DeleteImpl.DoesntExist", "Alexa implementation doesnt exist" );
  _impl.reset();
  SetAuthState( AlexaAuthState::Uninitialized );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnAlexaAuthChanged( AlexaAuthState state, const std::string& url, const std::string& code, bool errFlag )
{
  const auto oldState = _authState;
  bool codeExpired = false;
  
  // TODO (VIC-11517): downgrade. for now this is useful in webots
  LOG_WARNING( "Alexa.OnAlexaAuthChanged", "%d url='%s' code='%s'", (int)(state), url.c_str(), code.c_str() );
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
      TouchOptInFile();
      auto* mds = _context->GetMicDataSystem();
      if( mds != nullptr ) {
        const auto simpleState = (_uxState == AlexaUXState::Idle) ? AlexaSimpleState::Idle : AlexaSimpleState::Active;
        mds->SetAlexaState( simpleState );
      }
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
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetUXState( AlexaUXState newState )
{
  const auto oldState = _uxState;
  _uxState = newState;
  
  // send engine the UX state before starting any animations so that it knows what to do with animation end messages
  SendUXState();
  
  if( oldState != _uxState ) {
    // set backpack lights if streaming
    const bool listening = (_uxState == AlexaUXState::Listening);
    _context->GetBackpackLightComponent()->SetAlexaStreaming( listening );
  }
  
  if( _authState == AlexaAuthState::Authorized ) {
    auto* mds = _context->GetMicDataSystem();
    if( mds != nullptr ) {
      const auto simpleState = (_uxState == AlexaUXState::Idle) ? AlexaSimpleState::Idle : AlexaSimpleState::Active;
      mds->SetAlexaState( simpleState );
    }
  }
  
  if( (oldState == AlexaUXState::Idle) && (_uxState != AlexaUXState::Idle) ) {
    // when transitioning out of idle, play a getin animation, if there is one.
    auto* showStreamStateManager = _context->GetShowAudioStreamStateManager();
    if( showStreamStateManager != nullptr ) {
      if( showStreamStateManager->HasValidAlexaUXResponse( _uxState ) ) {
        showStreamStateManager->StartAlexaResponse( _uxState );
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnAlexaNetworkError( AlexaNetworkErrorType errorType )
{
  LOG_INFO( "Alexa.OnAlexaNetworkError.Type", "Error: %d", (int)errorType );
  _pendingUXState = _uxState;
  SetUXState( AlexaUXState::Error );
  PlayErrorAudio( errorType );
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
void Alexa::TouchOptInFile() const
{
  const auto& fullPath = GetOptInFilePath();
  Util::FileUtils::TouchFile( fullPath );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Alexa::DidAuthenticatePreviously() const
{
  const auto& fullPath = GetOptInFilePath();
  // todo (VIC-9837): ensure that when an account is unlinked, this file is deleted too
  const bool authenticatedPreviously = !fullPath.empty() && Util::FileUtils::FileExists( fullPath );
  return authenticatedPreviously;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::DeleteOptInFile() const
{
  const auto& fullPath = GetOptInFilePath();
  if( Util::FileUtils::FileExists( fullPath) ) {
    Util::FileUtils::DeleteFile( fullPath );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::DeleteUserFolder() const
{
  const auto* dataPlatform = _context->GetDataPlatform();
  const std::string fullPath = dataPlatform->pathToResource( Util::Data::Scope::Persistent,
                                                             kAlexaPath );
  if( Util::FileUtils::DirectoryExists( fullPath ) ) {
    Util::FileUtils::RemoveDirectory( fullPath );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string& Alexa::GetOptInFilePath() const
{
  static std::string fullPath;
  if( fullPath.empty() ) {
    const auto* dataPlatform = _context->GetDataPlatform();
    const std::string path = Util::FileUtils::AddTrailingFileSeparator(kAlexaPath) + kOptedInFile;
    fullPath = dataPlatform->pathToResource( Util::Data::Scope::Persistent,
                                             path.c_str() );
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
  if( _impl != nullptr ) {
    _impl->AddMicrophoneSamples( samples, nSamples );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::NotifyOfTapToTalk() const
{
  if( ANKI_VERIFY( _impl != nullptr,
                   "Alexa.NotifyOfTapToTalk.Disabled",
                   "Tap-to-talk was issued when alexa was disabled" ) )
  {
    _impl->NotifyOfTapToTalk();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::NotifyOfWakeWord( long from_ms, long to_ms ) const
{
  std::lock_guard<std::mutex> lg{ _implMutex };
  if( ANKI_VERIFY( _impl != nullptr,
                   "Alexa.NotifyOfWakeWord.Disabled",
                   "Wake word was issued when alexa was disabled" ) )
  {
    _impl->NotifyOfWakeWord( from_ms, to_ms );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::PlayErrorAudio( AlexaNetworkErrorType errorType )
{
  LOG_INFO( "Alexa.PlayErrorAudio.Type", "Setting error flag %d", (int) errorType );
  DEV_ASSERT( errorType != AlexaNetworkErrorType::NoError, "Alexa.PlayErrorAudio.NotAnError" );
  
  auto* audioController = _context->GetAudioController();
  if( !IsErrorPlaying() && (audioController != nullptr) ) {
    using namespace AudioEngine;
    auto* callbackContext = new AudioCallbackContext();
    callbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
    callbackContext->SetExecuteAsync( false ); // run on main thread
    callbackContext->SetEventCallbackFunc( [this]( const AudioCallbackContext* thisContext,
                                                   const AudioCallbackInfo& callbackInfo )
    {
      // reset error flag, then set the ux state with whatever the impl most recently sent as the ux state
      _timeToEndError_s = -1.0f;
      OnAlexaUXStateChanged( _pendingUXState );
    });
    
    const auto eventID = GetErrorAudioEvent( errorType );
    const auto gameObject = ToAudioGameObject(AudioMetaData::GameObjectType::Default);
    audioController->PostAudioEvent( eventID, gameObject, callbackContext );
  }
  
  // extend timeout
  _timeToEndError_s = kAlexaErrorTimeout_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}
  
  
} // namespace Vector
} // namespace Anki
