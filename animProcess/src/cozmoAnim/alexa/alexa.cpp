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
#include "clad/types/alexaAuthState.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
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
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// these are defined here since AlexaImpl is not defined in the header
Alexa::~Alexa() = default;
Alexa::Alexa(Alexa &&) noexcept = default;
Alexa& Alexa::operator=(Alexa &&) noexcept = default;
  
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
    // todo: tell micDataSystem to NOT use wakeword
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
  // set callbacks into this class (this one can occur during init)
  _impl->SetOnAlexaAuthStateChanged( std::bind( &Alexa::OnAlexaAuthChanged, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3 ) );
  const bool success = _impl->Init( _context );
  if( !success ) {
    // initialization was unsuccessful
    SetAlexaActive( false );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::DeleteImpl()
{
  ANKI_VERIFY( _impl != nullptr, "Alexa.DeleteImpl.DoesntExist", "Alexa implementation doesnt exist" );
  _impl.reset();
  SetAuthState( AlexaAuthState::Uninitialized );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::OnAlexaAuthChanged( AlexaAuthState state, const std::string& url, const std::string& code )
{
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
      }
    }
      break;
    case AlexaAuthState::Authorized:
    {
      // todo: tell micDataSystem to use wakeword
      SetAuthState( state );
      TouchOptInFile();
    }
      break;
    case AlexaAuthState::Invalid:
      break;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Alexa::SetAuthState( AlexaAuthState state, const std::string& url, const std::string& code )
{
  _authExtra = url;
  if( _authState != state ) {
    _authState = state;
    SendAuthState();
    
    const bool showAlexaFace = _authState == AlexaAuthState::WaitingForCode;
    if( showAlexaFace ) {
      std::string shortUrl;
      if( url.find("https://") != std::string::npos ) {
        shortUrl = url.substr(8);
      } else if( url.find("http://") != std::string::npos ) {
        shortUrl = url.substr(7);
      } else {
        shortUrl = url;
      }
      // note: order of params is flipped since url may or may not be displayed
      FaceInfoScreenManager::getInstance()->EnableAlexaScreen( showAlexaFace, code, shortUrl );
    } else {
      FaceInfoScreenManager::getInstance()->EnableAlexaScreen( false, "", "" );
    }
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
  if( _pendingEngineMsgs ) {
    _pendingEngineMsgs = false;
    SendAuthState();
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
    LOG_INFO( "Alexa.SetAuthState", "Setting state = %d", (int)_authState );
  } else {
    _pendingEngineMsgs = true;
    LOG_INFO( "Alexa.SetAuthState", "Pending state = %d", (int)_authState );
  }

  
}
} // namespace Vector
} // namespace Anki
