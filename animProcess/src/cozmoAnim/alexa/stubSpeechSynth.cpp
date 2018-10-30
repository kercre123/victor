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

#include "cozmoAnim/alexa/stubSpeechSynth.h"
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speechSynthesizer {

using namespace avsCommon::utils;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

static const std::string SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
static const std::string SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_NAME = "SpeechSynthesizer";
static const std::string SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_VERSION = "1.0";
static const std::string TAG{"SpeechSynthesizer"};
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)
static const std::string NAMESPACE{"SpeechSynthesizer"};
static const NamespaceAndName SPEAK{NAMESPACE, "Speak"};
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSpeechSynthesizerCapabilityConfiguration();

std::shared_ptr<AlexaSpeechSynthesizer> AlexaSpeechSynthesizer::create(std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender){
  auto speechSynthesizer = std::shared_ptr<AlexaSpeechSynthesizer>(new AlexaSpeechSynthesizer(exceptionSender));
    return speechSynthesizer;
}

avsCommon::avs::DirectiveHandlerConfiguration AlexaSpeechSynthesizer::getConfiguration() const {
    avsCommon::avs::DirectiveHandlerConfiguration configuration;
    configuration[SPEAK] = avsCommon::avs::BlockingPolicy::BLOCKING;
    return configuration;
}


  
void AlexaSpeechSynthesizer::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    ACSDK_DEBUG9(LX("handleDirectiveImmediately").d("messageId", directive->getMessageId()));
}

void AlexaSpeechSynthesizer::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("preHandleDirective").d("messageId", info->directive->getMessageId()));
}
  
void AlexaSpeechSynthesizer::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("handleDirective").d("messageId", info->directive->getMessageId()));
}

void AlexaSpeechSynthesizer::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("cancelDirective").d("messageId", info->directive->getMessageId()));
}

AlexaSpeechSynthesizer::AlexaSpeechSynthesizer(std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender)
  : CapabilityAgent{NAMESPACE, exceptionSender}
{
  m_capabilityConfigurations.insert(getSpeechSynthesizerCapabilityConfiguration());
}

std::shared_ptr<CapabilityConfiguration> getSpeechSynthesizerCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, SPEECHSYNTHESIZER_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaSpeechSynthesizer::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

}  // namespace speechSynthesizer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
