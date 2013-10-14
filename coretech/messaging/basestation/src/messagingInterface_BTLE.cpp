#include "anki/messaging/basestation/messagingInterface.h"

#ifdef USE_BTLE_MESSAGING_INTERFACE

#ifdef USE_TPE_MESSAGING_INTERFACE
#warning Both TCP and BTLE messaging interfaces defined!
#endif

namespace Anki {
  
  class MessagingInterface::Impl
  {
    
  public:
    Impl();
    
    Init();
    Run();
    
  private:
        
  }; // class MessagingInterface::Impl
  
  
  MessagingInterface::MessagingInterface(void)
  : pimpl( new MessagingInterface::Impl() )
  {
    
  }
  
} // namespace Anki

#endif // USE_BTLE_MESSAGING_INTERFACE