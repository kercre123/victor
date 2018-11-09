/**
 * File: alexaRevokeAuthHandler.h
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
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file an an analog to RevokeAuthroizationHandler.h, some portions were copied. Here's the SDK license
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

#ifndef ANIMPROCESS_COZMO_ALEXA_REVOKEAUTHORIZATIONHANDLER_H_
#define ANIMPROCESS_COZMO_ALEXA_REVOKEAUTHORIZATIONHANDLER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/RevokeAuthorizationObserverInterface.h>

namespace Anki {
namespace Vector {

class AlexaRevokeAuthHandler : public alexaClientSDK::avsCommon::avs::CapabilityAgent
{
public:
  static std::shared_ptr<AlexaRevokeAuthHandler>
  create( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender );

  alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
  void handleDirectiveImmediately(std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective> directive) override;
  void preHandleDirective(std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
  void handleDirective(std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
  void cancelDirective(std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
  
  bool addObserver(std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer);
  bool removeObserver(std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer);

private:
  AlexaRevokeAuthHandler( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender );

  void removeDirectiveGracefully( std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
                                  bool isFailure = false,
                                  const std::string& report = "" );

  std::mutex _mutex;

  // Observers to be notified when the System.RevokeAuthorization Directive has been received.
  std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface>> _revokeObservers;
};

}
}

#endif  // ANIMPROCESS_COZMO_ALEXA_REVOKEAUTHORIZATIONHANDLER_H_
