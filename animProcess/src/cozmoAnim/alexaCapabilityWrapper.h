// wraps a capability agent so that we can snoop on json directives
#ifndef ANIMPROCESS_COZMO_ALEXACAPABILITYWRAPPER_H
#define ANIMPROCESS_COZMO_ALEXACAPABILITYWRAPPER_H

#include <mutex>
#include <unordered_map>
//#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include "util/logging/logging.h"
#include <json/json.h>
#include <unordered_set>

namespace Anki {
namespace Vector {

  // todo: this should probably be a directive wrapper since that's what it's used for. CapabilityAgent methods are faster to override
class AlexaCapabilityWrapper
  : public alexaClientSDK::avsCommon::avs::CapabilityAgent
  //, public alexaClientSDK::avsCommon::sdkInterfaces::CapabilityConfigurationInterface
{
public:
  // todo: figure out how to get namespace from capability agent. the confguration can contain multiple namespaces
  AlexaCapabilityWrapper(const std::string& nameSpace,
                         std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent> capabilityAgent,
                         std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender )
   : CapabilityAgent( nameSpace, exceptionEncounteredSender)
   , m_capabilityAgent( capabilityAgent )
  {
  }
  
//  virtual std::unordered_set< std::shared_ptr< alexaClientSDK::avsCommon::avs::CapabilityConfiguration > > getCapabilityConfigurations () override
//  {
//    return m_capabilityAgent->getCapabilityConfigurations();
//  }
  
  virtual void preHandleDirective( std::shared_ptr< DirectiveInfo > info ) override
  {
    PRINT_NAMED_WARNING("WHATNOW", "preHandleDirective!");
    PRINT_NAMED_WARNING("WHATNOW", "preHandleDirective %s", info->directive != nullptr ? info->directive->getName().c_str() : "<NULL>");
    // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
    m_capabilityAgent->preHandleDirective(info);
  }
  
  virtual void handleDirective (std::shared_ptr< DirectiveInfo > info) override
  {
    LogDirective(info);
    // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
    m_capabilityAgent->handleDirective(info);
  }
  
  virtual void cancelDirective (std::shared_ptr< DirectiveInfo > info) override
  {
    // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
    m_capabilityAgent->cancelDirective(info);
  }
  
  virtual void handleDirectiveImmediately(std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective> directive) override {
    LogDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
    // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
    m_capabilityAgent->handleDirectiveImmediately(directive);
  }
  
  virtual alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override
  {
    PRINT_NAMED_WARNING("WHATNOW", "retrieving configuration!");
    // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
    return m_capabilityAgent->getConfiguration();
  }
  
private:
  
  void LogDirective(std::shared_ptr< DirectiveInfo > info) const
  {
    if( info == nullptr ) {
      PRINT_NAMED_WARNING("WHATNOW", "Received null directive");
      return;
    }
    
    if( info->directive != nullptr ) {
      PRINT_NAMED_WARNING("WHATNOW", "Received directive %s", info->directive->getName().c_str());
      const auto& payload = info->directive->getPayload();
      Json::Value json;
      Json::Reader reader;
      bool success = reader.parse(payload, json);
      if( success ) {
        PRINT_NAMED_WARNING("WHATNOW", "Received directive: %s", payload.c_str());
//        std::stringstream ss;
//        ss << json;
//        PRINT_NAMED_WARNING("WHATNOW", "Received directive:\n%s", ss.str().c_str());
      } else {
        PRINT_NAMED_WARNING("WHATNOW", "Could not parse into json!: %s", payload.c_str());
      }
    } else {
      PRINT_NAMED_WARNING("WHATNOW", "Received directive <NULL>");
    }
  }
  
  std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent> m_capabilityAgent;
  
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXACAPABILITYWRAPPER_H
