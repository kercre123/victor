/**
 * File: alexaKeywordObserver.h
 *
 * Author: ross
 * Created: Oct 20 2018
 *
 * Description: An implementation of the KeyWordObserverInterface that uses our special AlexaClient
 *              instead of the SDK's client
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

#ifndef ANIMPROCESS_COZMO_ALEXA_ALEXAKEYWORDOBSERVER_H
#define ANIMPROCESS_COZMO_ALEXA_ALEXAKEYWORDOBSERVER_H

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AIP/AudioProvider.h>
// todo: fwd declare
#include <AVSCommon/AVS/AudioInputStream.h>
#include <ESP/ESPDataProviderInterface.h>

namespace Anki {
namespace Vector {

class AlexaClient;

class AlexaKeywordObserver : public alexaClientSDK::avsCommon::sdkInterfaces::KeyWordObserverInterface
{
public:
    AlexaKeywordObserver( std::shared_ptr<AlexaClient> client,
                          alexaClientSDK::capabilityAgents::aip::AudioProvider audioProvider,
                          std::shared_ptr<alexaClientSDK::esp::ESPDataProviderInterface> espProvider = nullptr );

    virtual void onKeyWordDetected( std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream,
                                    std::string keyword,
                                    alexaClientSDK::avsCommon::avs::AudioInputStream::Index beginIndex = UNSPECIFIED_INDEX,
                                    alexaClientSDK::avsCommon::avs::AudioInputStream::Index endIndex = UNSPECIFIED_INDEX,
                                    std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr ) override;

private:
    std::shared_ptr<AlexaClient> _client;
    alexaClientSDK::capabilityAgents::aip::AudioProvider _audioProvider;
    std::shared_ptr<alexaClientSDK::esp::ESPDataProviderInterface> _espProvider;
};

}  // namespace Vector
}  // namespace Anki

#endif  // ANIMPROCESS_COZMO_ALEXA_ALEXAKEYWORDOBSERVER_H
