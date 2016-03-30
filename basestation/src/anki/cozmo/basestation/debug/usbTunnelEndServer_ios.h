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

#ifndef __iOS_usbTunnelEndServer_H__
#define __iOS_usbTunnelEndServer_H__

namespace Anki {
  namespace Util {
    namespace Data {
      class DataPlatform;
    }
  }
  namespace Cozmo {
    class IExternalInterface;
  }
}
#if TARGET_OS_IPHONE
void CreateUSBTunnelServer(Anki::Cozmo::IExternalInterface* externalInterface, Anki::Util::Data::DataPlatform* dataPlatform);
#else
void CreateUSBTunnelServer(Anki::Cozmo::IExternalInterface* externalInterface, Anki::Util::Data::DataPlatform* dataPlatform) {}
#endif
#endif // __iOS_usbTnnelEndServer_H__
