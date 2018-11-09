/**
 * File: alexaMessageRouter.cpp
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

#include "cozmoAnim/alexa/alexaMessageRouter.h"

namespace Anki {
namespace Vector {

using namespace alexaClientSDK;
#define LOG_CHANNEL "Alexa"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaMessageRouter::AlexaMessageRouter( const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface>>& observers,
                                        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                                        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
                                        std::shared_ptr<acl::TransportFactoryInterface> transportFactory,
                                        const std::string& avsEndpoint )
: MessageRouter{ authDelegate, attachmentManager, transportFactory, avsEndpoint }
, _observers{ observers }
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMessageRouter::sendMessage( std::shared_ptr<avsCommon::avs::MessageRequest> request )
{
  for( auto& observer : _observers ) {
    request->addObserver( observer );
  }
  MessageRouter::sendMessage( request );
}
  
}  // namespace Vector
}  // namespace Anki
