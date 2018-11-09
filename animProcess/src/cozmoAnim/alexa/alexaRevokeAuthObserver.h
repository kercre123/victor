/**
 * File: alexaRevokeAuthObserver.h
 *
 * Author: ross
 * Created: Nov 5 2018
 *
 * Description: Listens to onRevokeAuthorization and forwards it to the registrationManager
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file an an analog to AlexaRevokeAuthObserver.cpp, some portions were copied. Here's the SDK license
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

#include <AVSCommon/SDKInterfaces/RevokeAuthorizationObserverInterface.h>
#include <RegistrationManager/RegistrationManager.h>

#ifndef ANIMPROCESS_COZMO_ALEXA_ALEXAREVOKEAUTHOBSERVER_H
#define ANIMPROCESS_COZMO_ALEXA_ALEXAREVOKEAUTHOBSERVER_H

namespace Anki {
namespace Vector {

class AlexaRevokeAuthObserver : public alexaClientSDK::avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface
{
public:
  
    explicit AlexaRevokeAuthObserver( std::shared_ptr<alexaClientSDK::registrationManager::RegistrationManager> manager );
    void onRevokeAuthorization() override;

private:
    std::shared_ptr<alexaClientSDK::registrationManager::RegistrationManager> _manager;
};

}
}

#endif  // ANIMPROCESS_COZMO_ALEXA_ALEXAREVOKEAUTHOBSERVER_H
