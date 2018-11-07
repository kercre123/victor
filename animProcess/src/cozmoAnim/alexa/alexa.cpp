/**
 * File: alexa.cpp
 *
 * Author: ross
 * Created: Oct 16 2018
 *
 * Description: Wrapper for component that integrates the Alexa Voice Service (AVS) SDK. Alexa is an opt-in
 *              feature, so this class handles communication with the engine to opt in and out, and is
 *              otherwise primarily a pimpl wrapper.
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#include "cozmoAnim/alexa/alexa.h"
#include "cozmoAnim/alexa/alexaImpl.h" // impl declaration
#include "cozmoAnim/animProcessMessages.h"
#include "clad/types/alexaTypes.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenTypes.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/showAudioStreamStateManager.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"



namespace Anki {
namespace Vector {
  
namespace {
  const std::string kAlexaPath = "alexa";
  const std::string kOptedInFile = "optedIn";
  #define LOG_CHANNEL "Alexa"
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Alexa::Alexa()
: _authState{ AlexaAuthState::Uninitialized }
, _uxState{ AlexaUXState::Idle }
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
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetAlexaUsage(bool optedIn)
{
  LOG_INFO( "Alexa.SetAlexaUsage.Opting",
            "User requests to log %s",
            optedIn ? "in" : "out" );
  
  _userOptedIn = optedIn;
  if( !optedIn && HasImpl() ) {
    // TODO (VIC-9837): tell impl to delete customer data
  }
  SetAlexaActive( optedIn );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetAlexaActive(bool active)
{
  LOG_INFO( "Alexa.SetAlexaActive", "Internally active = %d", active );
  
  if( active && !HasImpl() ) {
    CreateImpl();
  } else if( !active && HasImpl() ) {
    DeleteImpl();
    
    auto* mds = _context->GetMicDataSystem();
    if( mds != nullptr ) {
      mds->SetAlexaActive( false );
    }
  }
  
  if( !active ) {
    DeleteOptInFile();
    // this is also set in other ways, but just to be sure
    SetAuthState( AlexaAuthState::Uninitialized );
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
  const bool success = _impl->Init( _context );
  if( !success ) {
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
  
  LOG_INFO( "Alexa.OnAlexaAuthChanged", "%d url='%s' code='%s'", (int)(state), url.c_str(), code.c_str() );
  switch( state ) {
    case AlexaAuthState::Uninitialized:
    {
      SetAlexaActive( false ); // which also sets the state
    }
      break;
    case AlexaAuthState::RequestingAuth:
    {
      SetAuthState( state );
    }
      break;
    case AlexaAuthState::WaitingForCode:
    {
      if( !_userOptedIn ) {
        // user didn't request to auth during this session, which means that an old token has expired. Logout
        SetAlexaActive( false );
      } else if( !code.empty() && ((_previousCode == code) || _previousCode.empty()) ) {
        LOG_INFO( "Alexa.OnAlexaAuthChanged.FirstCode", "Received code '%s'", code.c_str() );
        _previousCode = code;
        SetAuthState( state, url, code );
      } else {
        LOG_DEBUG( "Alexa.OnAlexaAuthChanged.CodeRefresh", "Received another code (%s)", code.c_str());
        // this is not the first code, which means the first one expired. we don't have a good way of telling
        // the user that, so cancel authentication
        SetAlexaActive( false );
        codeExpired = true;
      }
    }
      break;
    case AlexaAuthState::Authorized:
    {
      SetAuthState( state );
      TouchOptInFile();
      auto* mds = _context->GetMicDataSystem();
      if( mds != nullptr ) {
        mds->SetAlexaActive( true );
      }
    }
      break;
    case AlexaAuthState::Invalid:
      break;
  }
  
  
  // set face info screen. only show faces if the user opted in via app or voice command
  if( _userOptedIn ) {
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
    if( url.find("https://") != std::string::npos ) {
      url = url.substr(8);
    } else if( url.find("http://") != std::string::npos ) {
      url = url.substr(7);
    }
    // note: order of params is flipped since url may or may not be displayed
    FaceInfoScreenManager::getInstance()->EnableAlexaScreen( screenName, code, url );
  } else {
    FaceInfoScreenManager::getInstance()->EnableAlexaScreen( screenName, "", "" );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnAlexaUXStateChanged( AlexaUXState newState )
{
  const auto oldState = _uxState;
  _uxState = newState;
  
  // send engine the UX state before starting any animations so that it knows what to do with animation end events
  SendUXState();
  
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
void Alexa::SendUXState()
{
  if( _engineLoaded ) {
    RobotInterface::AlexaUXChanged msg;
    msg.state = _uxState;
    RobotInterface::SendAnimToEngine( msg );
    LOG_INFO( "Alexa.SendUXState", "Sending state = %d", (int)_uxState );
  } else {
    // the ux state can change before engine init if a timer goes off, for example
    _pendingUXMsgs = true;
    LOG_INFO( "Alexa.SendUXState", "Pending state = %d", (int)_uxState );
  }
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
    LOG_INFO( "Alexa.SendAuthState", "Sending state = %d", (int)_authState);
  } else {
    _pendingAuthMsgs = true;
    LOG_INFO( "Alexa.SendAuthState", "Pending state = %d", (int)_authState);
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
  if( ANKI_VERIFY( _impl != nullptr,
                   "Alexa.NotifyOfWakeWord.Disabled",
                   "Wake word was issued when alexa was disabled" ) )
  {
    _impl->NotifyOfWakeWord( from_ms, to_ms );
  }
}
  
} // namespace Vector
} // namespace Anki
