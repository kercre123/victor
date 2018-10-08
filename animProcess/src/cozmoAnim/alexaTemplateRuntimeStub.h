// pretends we offer a TemplateRuntime capability agent so the wrapper gets template runtime metadata
#ifndef ANIMPROCESS_COZMO_ALEXATEMPLATERUNTIMESTUB_H
#define ANIMPROCESS_COZMO_ALEXATEMPLATERUNTIMESTUB_H

#include <unordered_map>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {

class AlexaTemplateRuntimeStub
  : public alexaClientSDK::avsCommon::avs::CapabilityAgent
  , public alexaClientSDK::avsCommon::sdkInterfaces::CapabilityConfigurationInterface
{
private:
  const std::string NAMESPACE{"TemplateRuntime"};
  const std::string RENDER_TEMPLATE{"RenderTemplate"};
  const std::string RENDER_PLAYER_INFO{"RenderPlayerInfo"};
  const alexaClientSDK::avsCommon::avs::NamespaceAndName TEMPLATE{NAMESPACE, RENDER_TEMPLATE};
  const alexaClientSDK::avsCommon::avs::NamespaceAndName PLAYER_INFO{NAMESPACE, RENDER_PLAYER_INFO};
  const std::string TEMPLATERUNTIME_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
  const std::string TEMPLATERUNTIME_CAPABILITY_INTERFACE_NAME = "TemplateRuntime";
  const std::string TEMPLATERUNTIME_CAPABILITY_INTERFACE_VERSION = "1.1";
  
public:
  explicit AlexaTemplateRuntimeStub(std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender)
   : CapabilityAgent( NAMESPACE, exceptionEncounteredSender)
  {
  }
  
  virtual void preHandleDirective( std::shared_ptr< DirectiveInfo > info ) override{}
  
  virtual void handleDirective (std::shared_ptr< DirectiveInfo > info) override{}
  
  virtual void cancelDirective (std::shared_ptr< DirectiveInfo > info) override{}
  
  virtual void handleDirectiveImmediately(std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective> directive) override{}
  
  virtual alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override
  {
    alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration configuration;
    configuration[TEMPLATE] = alexaClientSDK::avsCommon::avs::BlockingPolicy::HANDLE_IMMEDIATELY;
    configuration[PLAYER_INFO] = alexaClientSDK::avsCommon::avs::BlockingPolicy::HANDLE_IMMEDIATELY;
    return configuration;
  }
  
  virtual std::unordered_set< std::shared_ptr< alexaClientSDK::avsCommon::avs::CapabilityConfiguration > >
    getCapabilityConfigurations () override
  {
    using namespace alexaClientSDK::avsCommon::avs;
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY,
                      TEMPLATERUNTIME_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY,
                      TEMPLATERUNTIME_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY,
                      TEMPLATERUNTIME_CAPABILITY_INTERFACE_VERSION});
    
    std::unordered_set< std::shared_ptr< CapabilityConfiguration > > ret;
    ret.insert( std::make_shared<CapabilityConfiguration>(configMap) );
    return ret;
  }
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXATEMPLATERUNTIMESTUB_H
