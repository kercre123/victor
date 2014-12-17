//
//  advertisementService.h
//

#ifndef __ADVERTISEMENT_SERVICE_H__
#define __ADVERTISEMENT_SERVICE_H__

#include <map>
#include <anki/messaging/shared/UdpServer.h>

namespace Anki {
  namespace Comms {
    
    typedef struct {
      unsigned short port;         // Port that advertising device is accepting connections on
      unsigned char ip[18];        // IP address as null terminated string
      unsigned char id;
      unsigned char enableAdvertisement;  // 1 = register, 0 = deregister
    } AdvertisementRegistrationMsg;
    
    typedef struct  {
      unsigned short port;
      unsigned char ip[17];        // IP address as null terminated string
      unsigned char id;
    } AdvertisementMsg;
    

    class AdvertisementService
    {
    public:
      
      static const int MAX_SERVICE_NAME_LENGTH = 64;
      
      AdvertisementService(const char* serviceName);
      
      // registrationPort:  Port on which to accept registration messages
      //                    from devices that want to advertise.
      // advertisementPort: Port on which to accept clients that want to
      //                    receive advertisements
      void StartService(int registrationPort, int advertisementPort);
      
      // Stops listening for clients and clears all registered advertisers and advertisement listeners.
      void StopService();
      
      // This needs to be called at the frequency you want to accept registrations and advertise.
      // TODO: Perhaps StartService() should launch a thread to just do this internally.
      void Update();
      
      // Exposed so that you can force-add an advertiser via API
      void ProcessRegistrationMsg(const AdvertisementRegistrationMsg &msg);
      
      
    protected:

      char serviceName_[MAX_SERVICE_NAME_LENGTH];
      
      // Device that want to advertise connect to this server
      UdpServer regServer_;
      
      // Devices that want to receive advertisements connect to this server
      UdpServer advertisingServer_;
      
      // Map of advertising device id to AdvertisementMsg
      typedef std::map<int, AdvertisementMsg>::iterator connectionInfoMapIt;
      std::map<int, AdvertisementMsg> connectionInfoMap_;
      

    };
    
  }
}

#endif // ADVERTISEMENT_SERVICE_H
