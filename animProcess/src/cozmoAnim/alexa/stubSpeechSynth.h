// A stub for the SpeechSynthesizer capability so that alexa will authorize.
// Otherwise you have to make media players simply to authorize.
// This will get removed in the next commit or two.
// No need for PR review



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

#ifndef COZMOANIM_SPEECH_SYNTHESIZER_H_
#define COZMOANIM_SPEECH_SYNTHESIZER_H_
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <deque>

#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speechSynthesizer {

class AlexaSpeechSynthesizer
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public std::enable_shared_from_this<AlexaSpeechSynthesizer>
{
public:
  
  void shutdown() {}
  
  static std::shared_ptr<AlexaSpeechSynthesizer> create(std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

  avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;

  
  void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;

  void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;

  void handleDirective(std::shared_ptr<DirectiveInfo> info) override;

  void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;

  std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;

private:

  explicit AlexaSpeechSynthesizer(std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

  std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

};

}  // namespace speechSynthesizer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // COZMOANIM_SPEECH_SYNTHESIZER_H_
