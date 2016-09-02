/**
 * File: usbTunnelEndServer.h
 *
 * Author: Molly Jameson
 * Created: 03/28/16
 *
 * Description: Handles the ios specific server function for usb tunnel
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Debug_DebugConsoleManager_usbTunnelEndServer_H__
#define __Cozmo_Basestation_Debug_DebugConsoleManager_usbTunnelEndServer_H__

#include "util/global/globalDefinitions.h"

#if ANKI_DEV_CHEATS
#include "util/helpers/noncopyable.h"
#include <memory>


namespace Anki {
  
  namespace Util {
    namespace Data {
      class DataPlatform;
    }
  }
  
  namespace Cozmo {
    
    class IExternalInterface;
    // Super lightweight wrapper.
    class USBTunnelServer : Util::noncopyable
    {
    public:
      USBTunnelServer(Anki::Cozmo::IExternalInterface* externalInterface, Anki::Util::Data::DataPlatform* dataPlatform);
      virtual ~USBTunnelServer();
      
      static const std::string TempAnimFileName;
      static const std::string FaceAnimsDir;
      
    private:
      // pImlp to keep out nasty obj-c
      struct USBTunnelServerImpl;
      std::unique_ptr<USBTunnelServerImpl> _impl;
    }; // class USBTunnelServer
  } // namespace Cozmo
} // namespace Anki

#endif //ANKI_DEV_CHEATS


#endif // __Cozmo_Basestation_Debug_DebugConsoleManager_usbTunnelEndServer_H__
