/*
 * File:          advertisementService.cpp
 * Date:
 * Description:   A service with which devices can register at the registration port if they want to 
 *                advertise to presence to others.
 *                Listener devices can connect to the advertisementPort if they want to see 
 *                advertising devices.
 *
 * Author:        Kevin Yoon
 */

#include <iostream>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <anki/messaging/basestation/advertisementService.h>


namespace Anki {
  namespace Comms {

    AdvertisementService::AdvertisementService(const char* serviceName)
    {
      strcpy(serviceName_, serviceName);
    }

    void AdvertisementService::StartService(int registrationPort, int advertisementPort)
    {
      // Start listening for clients that want to advertise
      regServer_.StartListening(registrationPort);
      
      // Start listening for clients that want to receive advertisements
      advertisingServer_.StartListening(advertisementPort);
    }
    
    void AdvertisementService::StopService()
    {
      regServer_.StopListening();
      advertisingServer_.StopListening();
      
      connectionInfoMap_.clear();
    }
    
    void AdvertisementService::Update()
    {
      // Message from device that wants to (de)register for advertising.
      AdvertisementRegistrationMsg regMsg;
  
      connectionInfoMapIt it;
  
      //double lastAdvertisingTime = 0;
  
      
      // Update registered devices
      int bytes_recvd = 0;
      do {
        // For now, assume that only one kind of message comes in
        bytes_recvd = regServer_.Recv((char*)&regMsg, sizeof(regMsg));
    
        if (bytes_recvd == sizeof(regMsg)) {
          ProcessRegistrationMsg(regMsg);
        } else if (bytes_recvd > 0){
          printf("Recvd datagram with %d bytes. Expecting %d bytes.\n", bytes_recvd, (int)sizeof(regMsg));
        }
        
      } while (bytes_recvd > 0);

      
      // Get clients that are interested in knowing about advertising devices
      do {
        // Don't actually expect to get AdvertisementRegistrationMsg here,
        // but just need something to put stuff in.
        // Server automatically adds client to internal list when recv is called.
        bytes_recvd = advertisingServer_.Recv((char*)&regMsg, sizeof(regMsg));
        
        //if (bytes_recvd > 0) {
        //  std::cout << serviceName_ << ": " << "Received ping from advertisement listener\n";
        //}
      } while(bytes_recvd > 0);
      
      
      // Notify all clients of advertising devices
      if (advertisingServer_.GetNumClients() > 0 && (!connectionInfoMap_.empty() || !oneShotAdvertiseConnectionInfoMap_.empty() )) {
        
        std::cout << serviceName_ << ": "
                  << "Notifying " <<  advertisingServer_.GetNumClients() << " clients of advertising devices\n";
        
        // Send registered devices' advertisement
        for (it = connectionInfoMap_.begin(); it != connectionInfoMap_.end(); it++) {
          
          std::cout << serviceName_ << ": "
                    << "Advertising: Device " << it->second.id
                    << " on host " << it->second.ip
                    << " at port " << it->second.port
                    << "(size=" << sizeof(AdvertisementMsg) << ")\n";
           
          advertisingServer_.Send((char*)&(it->second), sizeof(AdvertisementMsg));
        }
        
        // Send one-shot advertisements
        for (it = oneShotAdvertiseConnectionInfoMap_.begin(); it != oneShotAdvertiseConnectionInfoMap_.end(); it++) {

          std::cout << serviceName_ << ": "
          << "Advertising (one-shot): Device " << int(it->second.id)
          << " on host " << it->second.ip
          << " at port " << it->second.port
          << "(size=" << sizeof(AdvertisementMsg) << ")\n";
          
          advertisingServer_.Send((char*)&(it->second), sizeof(AdvertisementMsg));
        }
        
        // Clearing advertising devices
        oneShotAdvertiseConnectionInfoMap_.clear();
      }
      
    }
    
  
    void AdvertisementService::ProcessRegistrationMsg(const AdvertisementRegistrationMsg &regMsg)
    {
      if (regMsg.enableAdvertisement) {
        
        if (regMsg.oneShot) {
          std::cout << serviceName_ << ": "
          << "One-shot advertising device " << (int)regMsg.id
          << " on host " << regMsg.ip
          << " at port " << regMsg.port
          << " with advertisement service\n";

          oneShotAdvertiseConnectionInfoMap_[regMsg.id].id = regMsg.id;
          oneShotAdvertiseConnectionInfoMap_[regMsg.id].port = regMsg.port;
          oneShotAdvertiseConnectionInfoMap_[regMsg.id].protocol = regMsg.protocol;
          memcpy(oneShotAdvertiseConnectionInfoMap_[regMsg.id].ip, regMsg.ip, sizeof(AdvertisementMsg::ip));
        } else {
          std::cout << serviceName_ << ": "
          << "Registering device " << (int)regMsg.id
          << " on host " << regMsg.ip
          << " at port " << regMsg.port
          << " with advertisement service\n";

          connectionInfoMap_[regMsg.id].id = regMsg.id;
          connectionInfoMap_[regMsg.id].port = regMsg.port;
          connectionInfoMap_[regMsg.id].protocol = regMsg.protocol;
          memcpy(connectionInfoMap_[regMsg.id].ip, regMsg.ip, sizeof(AdvertisementMsg::ip));
        }
        
      } else {
        std::cout << serviceName_ << ": "
                  << "Deregistering device " << (int)regMsg.id << " from advertisement service\n";
        connectionInfoMap_.erase(regMsg.id);
      }

    }
  
    void AdvertisementService::DeregisterAllAdvertisers()
    {
      connectionInfoMap_.clear();
    }
    
    
  }  // namespace Comms
}  // namespace Anki
