/**
 * File: alexaCapabilityWrapper.cpp
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

#include "cozmoAnim/alexa/alexaCapabilityWrapper.h"
#include <memory>

namespace Anki {
namespace Vector {
  
using namespace alexaClientSDK;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaCapabilityWrapper::AlexaCapabilityWrapper( const std::string& nameSpace,
                                                std::shared_ptr<avsCommon::avs::CapabilityAgent> capabilityAgent,
                                                std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
                                                const OnDirectiveFunc& onDirective )
: CapabilityAgent( nameSpace, exceptionEncounteredSender)
, _capabilityAgent( capabilityAgent )
, _onDirective( onDirective )
{
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaCapabilityWrapper::preHandleDirective( std::shared_ptr<DirectiveInfo> info )
{
  // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
  _capabilityAgent->preHandleDirective(info);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaCapabilityWrapper::handleDirective( std::shared_ptr<DirectiveInfo> info )
{
  if( info != nullptr ) {
    RunDirectiveCallback(info->directive);
  }
  // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
  _capabilityAgent->handleDirective(info);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaCapabilityWrapper::cancelDirective( std::shared_ptr<DirectiveInfo> info )
{
  // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
  // TODO: implement cancel directive callback. the ones we care about (time) come with separate directives)
  _capabilityAgent->cancelDirective(info);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaCapabilityWrapper::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive)
{
  // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
  RunDirectiveCallback( directive );
  
  _capabilityAgent->handleDirectiveImmediately(directive);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
avsCommon::avs::DirectiveHandlerConfiguration AlexaCapabilityWrapper::getConfiguration() const
{
  // note: this AVS method was made public only for the purpose of this wrapper, so if you delete the wrapper, revert the libs
  return _capabilityAgent->getConfiguration();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaCapabilityWrapper::RunDirectiveCallback( const std::shared_ptr<avsCommon::avs::AVSDirective>& directive )
{
  if( _onDirective != nullptr && directive != nullptr ) {
    const auto& name = directive->getName();
    const auto& payload = directive->getPayload();
    const auto& unparsed = directive->getUnparsedDirective();
    _onDirective( name, payload, unparsed );
  }
}
  
  
} // namespace Vector
} // namespace Anki
