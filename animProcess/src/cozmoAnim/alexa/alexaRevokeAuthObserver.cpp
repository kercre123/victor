/**
 * File: alexaRevokeAuthObserver.cpp
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

#include "cozmoAnim/alexa/alexaRevokeAuthObserver.h"

namespace Anki {
namespace Vector {

namespace {
  using namespace alexaClientSDK;
  static const std::string TAG("AlexaRevokeAuthObserver");

  #define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaRevokeAuthObserver::AlexaRevokeAuthObserver( std::shared_ptr<registrationManager::RegistrationManager> manager )
  : _manager{ manager }
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaRevokeAuthObserver::onRevokeAuthorization()
{
  if( _manager ) {
    _manager->logout();
  }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
