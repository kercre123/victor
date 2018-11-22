/**
 * File: alexaMessageRouter.h
 *
 * Author: ross
 * Created: Nov 4 2018
 *
 * Description: Subclass of the MessageRouter that accepts MessageRequestObserverInterface observers.
 *              We need this to receive onSendCompleted, which provides error status on failed
 *              attempts to send data to AVS.
 *              
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file uses the sdk, here's the SDK license
/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ANIMPROCESS_COZMO_ALEXA_ALEXAMESSAGEROUTER_H
#define ANIMPROCESS_COZMO_ALEXA_ALEXAMESSAGEROUTER_H

#include <ACL/Transport/MessageRouter.h>
// todo: fwd declare where possible
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include "AVSCommon/SDKInterfaces/AuthDelegateInterface.h"
#include "ACL/Transport/MessageConsumerInterface.h"
#include "ACL/Transport/MessageRouterInterface.h"
#include "ACL/Transport/MessageRouterObserverInterface.h"
#include "ACL/Transport/TransportFactoryInterface.h"
#include "ACL/Transport/TransportInterface.h"


namespace Anki {
namespace Vector {
    
class AlexaMessageRouter : public alexaClientSDK::acl::MessageRouter
{
public:
  
  AlexaMessageRouter( const std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface>>& observers,
                      std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                      std::shared_ptr<alexaClientSDK::avsCommon::avs::attachment::AttachmentManager> attachmentManager,
                      std::shared_ptr<alexaClientSDK::acl::TransportFactoryInterface> transportFactory,
                      const std::string& avsEndpoint = "" );
  
  virtual void sendMessage( std::shared_ptr<alexaClientSDK::avsCommon::avs::MessageRequest> request ) override;

  virtual void consumeMessage(const std::string& contextId, const std::string& message) override;
  
private:
  
  std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface>> _observers;
};

  
} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_ALEXAMESSAGEROUTER_H
