/**
 * File: alexaRevokeAuthHandler.cpp
 *
 * Author: ross
 * Created: Nov 5 2018
 *
 * Description: Capability agent that reacts to a device being de-authorized server-side
 *              NOTE: there's no reason to have this file. Alexa's SDK provides an almost identical
 *              copy, other than formatting. This can be enabled by adding "add_definitions(-DENABLE_REVOKE_AUTH)"
 *              to the build settings cmake file and rebuilding the sdk. However, I then get
 *              linker errors when building the sdk. For now, this file exists.
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file an an analog to RevokeAuthroizationHandler.cpp, some portions were copied. Here's the SDK license
/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "cozmoAnim/alexa/alexaRevokeAuthHandler.h"

#include <string>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/SDKInterfaces/RevokeAuthorizationObserverInterface.h>

namespace Anki {
namespace Vector {

using namespace alexaClientSDK;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
  
namespace {
  // String to identify log entries originating from this file.
  static const std::string TAG{"AlexaRevokeAuthHandler"};

  #define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

  // This string holds the namespace for AVS revoke.
  static const std::string REVOKE_NAMESPACE = "System";

  // This string holds the name of the directive that's being sent for revoking.
  static const std::string REVOKE_DIRECTIVE_NAME = "RevokeAuthorization";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::shared_ptr<AlexaRevokeAuthHandler> AlexaRevokeAuthHandler::create( std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender)
{
  ACSDK_DEBUG5(LX(__func__));
  if( !exceptionEncounteredSender ) {
    ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
    return nullptr;
  }
  return std::shared_ptr<AlexaRevokeAuthHandler>( new AlexaRevokeAuthHandler( exceptionEncounteredSender ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaRevokeAuthHandler::AlexaRevokeAuthHandler( std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender )
: CapabilityAgent(REVOKE_NAMESPACE, exceptionEncounteredSender)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
avsCommon::avs::DirectiveHandlerConfiguration AlexaRevokeAuthHandler::getConfiguration() const
{
  return avsCommon::avs::DirectiveHandlerConfiguration{ {NamespaceAndName{REVOKE_NAMESPACE, REVOKE_DIRECTIVE_NAME},
                                                         avsCommon::avs::BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)}};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaRevokeAuthHandler::preHandleDirective( std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaRevokeAuthHandler::handleDirectiveImmediately( std::shared_ptr<AVSDirective> directive )
{
  handleDirective( std::make_shared<avsCommon::avs::CapabilityAgent::DirectiveInfo>(directive, nullptr) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaRevokeAuthHandler::handleDirective( std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info ) {
  if( !info || !info->directive ) {
    ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveOrDirectiveInfo"));
    return;
  }

  std::unique_lock<std::mutex> lock( _mutex );
  auto observers = _revokeObservers;
  lock.unlock();

  for( auto observer : observers ) {
    observer->onRevokeAuthorization();
  }

  removeDirectiveGracefully( info );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaRevokeAuthHandler::cancelDirective( std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info )
{
  if( !info || !info->directive ) {
    removeDirectiveGracefully( info, true, "nullDirective" );
    ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullDirectiveOrDirectiveInfo"));
    return;
  }
  CapabilityAgent::removeDirective( info->directive->getMessageId() );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaRevokeAuthHandler::removeDirectiveGracefully( std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
                                                        bool isFailure,
                                                        const std::string& report )
{
  if( info ) {
    if( info->result ) {
      if( isFailure ) {
        info->result->setFailed(report);
      } else {
        info->result->setCompleted();
      }
      if( info->directive ) {
        CapabilityAgent::removeDirective( info->directive->getMessageId() );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaRevokeAuthHandler::addObserver( std::shared_ptr<RevokeAuthorizationObserverInterface> observer )
{
  ACSDK_DEBUG5(LX(__func__));
  if (!observer) {
    ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
    return false;
  }

  std::lock_guard<std::mutex> lock( _mutex );
  return _revokeObservers.insert( observer ).second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaRevokeAuthHandler::removeObserver( std::shared_ptr<RevokeAuthorizationObserverInterface> observer )
{
  if (!observer) {
    ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
    return false;
  }

  std::lock_guard<std::mutex> lock(_mutex);
  return (_revokeObservers.erase( observer ) == 1);
}

}
}
