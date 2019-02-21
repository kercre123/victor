/**
 * File: alexaCapabilityWrapper.h
 *
 * Author: ross
 * Created: Oct 19 2018
 *
 * Description: Wraps capabilities so that we can snoop on their directives
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#ifndef ANIMPROCESS_COZMO_ALEXA_ALEXACAPABILITYWRAPPER_H
#define ANIMPROCESS_COZMO_ALEXA_ALEXACAPABILITYWRAPPER_H

#include <AVSCommon/AVS/CapabilityAgent.h>

namespace Anki {
namespace Vector {

  
class AlexaCapabilityWrapper : public alexaClientSDK::avsCommon::avs::CapabilityAgent
{
public:
  using OnDirectiveFunc = std::function<void(const std::string&,const std::string&,const std::string&)>;
  AlexaCapabilityWrapper( const std::string& nameSpace,
                          std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent> capabilityAgent,
                          std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
                          const OnDirectiveFunc& onDirective );
  
  virtual void preHandleDirective( std::shared_ptr<DirectiveInfo> info ) override;
  
  virtual void handleDirective( std::shared_ptr<DirectiveInfo> info ) override;
  
  virtual void cancelDirective( std::shared_ptr<DirectiveInfo> info ) override;
  
  virtual void handleDirectiveImmediately( std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective> directive ) override;
  
  virtual alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
  
private:
  
  void RunDirectiveCallback( const std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective>& directive );
  
  std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent> _capabilityAgent;
  
  const OnDirectiveFunc& _onDirective;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_ALEXACAPABILITYWRAPPER_H
