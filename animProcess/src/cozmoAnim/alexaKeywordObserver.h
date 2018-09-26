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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_KEYWORDOBSERVER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_KEYWORDOBSERVER_H_

#include <memory>
#include <string>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <DefaultClient/DefaultClient.h>
#include <ESP/ESPDataProviderInterface.h>

namespace Anki {
namespace Vector {

class AlexaClient;
/**
 * Observes callbacks from keyword detections and notifies the DefaultClient that a wake word has occurred.
 */
  class KeywordObserver : public alexaClientSDK::avsCommon::sdkInterfaces::KeyWordObserverInterface {
public:
    /**
     * Constructor.
     *
     * @param client The default SDK client.
     * @param audioProvider The audio provider from which to stream audio data from.
     * @parm espProvider The ESP provider to calculate the Ambient and Voice energy from the audio stream.
     */
    KeywordObserver(
        std::shared_ptr<AlexaClient> client,
        alexaClientSDK::capabilityAgents::aip::AudioProvider audioProvider,
        std::shared_ptr<alexaClientSDK::esp::ESPDataProviderInterface> espProvider = nullptr);

    /// @name KeyWordObserverInterface Functions
    /// @{
    void onKeyWordDetected(
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream,
        std::string keyword,
        alexaClientSDK::avsCommon::avs::AudioInputStream::Index beginIndex = UNSPECIFIED_INDEX,
        alexaClientSDK::avsCommon::avs::AudioInputStream::Index endIndex = UNSPECIFIED_INDEX,
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr) override;
    /// @}

private:
    /// The default SDK client.
    std::shared_ptr<AlexaClient> m_client;

    /// The audio provider.
    alexaClientSDK::capabilityAgents::aip::AudioProvider m_audioProvider;

    /// Echo Spatial Perception (ESP) provider.
    std::shared_ptr<alexaClientSDK::esp::ESPDataProviderInterface> m_espProvider;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_KEYWORDOBSERVER_H_

