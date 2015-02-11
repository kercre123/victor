//
//  advertisementService.h
//

#ifndef __ADVERTISEMENT_SERVICE_H__
#define __ADVERTISEMENT_SERVICE_H__

#include <map>
#include <anki/messaging/shared/UdpServer.h>

namespace Anki {
  namespace Comms {
    
    typedef enum {
      TCP = 0,
      UDP
    } Protocol;
    
    // Message received from device at registrationPort that wants to advertise.
    // If enableAdvertisement == 1 and oneShot == 0, the device is registered to the service
    // which will then advertise for the device on subsequent calls to Update().
    // If enableAdvertisement == 1 and oneShot == 1, the service will advertise one time
    // on the next call to Update(). This mode is helpful in that an advertising device
    // need not know whether an advertisement service is running before it sends a registration message.
    // It just keeps sending them!
    // If enableAdvertisement == 0, the device is deregistered if it isn't already.
    typedef struct {
      unsigned short port;         // Port that advertising device is accepting connections on
      unsigned char ip[18];        // IP address as null terminated string
      unsigned char id;
      unsigned char protocol;      // See Protocol enum
      unsigned char enableAdvertisement;  // 1 = register, 0 = deregister
      unsigned char oneShot;
    } AdvertisementRegistrationMsg;
    
    // Sent to all clients interested in knowing about advertising devices from advertisementPort.
    typedef struct  {
      unsigned short port;
      unsigned char ip[17];        // IP address as null terminated string
      unsigned char id;
      unsigned char protocol;      // See Protocol enum
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

      // Clears the list of advertising devices
      void DeregisterAllAdvertisers();
      
    protected:

      char serviceName_[MAX_SERVICE_NAME_LENGTH];
      
      // Devices that want to advertise connect to this server
      UdpServer regServer_;
      
      // Devices that want to receive advertisements connect to this server
      UdpServer advertisingServer_;
      
      // Map of advertising device id to AdvertisementMsg
      // populated by AdvertisementRegistrationMsg
      typedef std::map<int, AdvertisementMsg>::iterator connectionInfoMapIt;
      std::map<int, AdvertisementMsg> connectionInfoMap_;
      
      // Map of advertising device id to AdvertisementMsg
      // for one-shot advertisements, also populated by AdvertisementRegistrationMsg
      std::map<int, AdvertisementMsg> oneShotAdvertiseConnectionInfoMap_;
      

    };
    
  }
}

#endif // ADVERTISEMENT_SERVICE_H
