//
//  ViewController.m
//  ios-server
//
//  Created by Paul Aluri on 1/9/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import "ViewController.h"
#include <string>
#include <iostream>
#include <NetworkExtension/NetworkExtension.h>
#include "networking/BLEPairingController.h"
#include "networking/BLENetworkStream.h"

@interface ViewController () {
  BLEPairingController* pairingController;
}

@property (weak, nonatomic) IBOutlet UILabel *pinLabel;
@property (weak, nonatomic) IBOutlet UILabel *sharedSecretLabel;

@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  // Create six digit pin
  pairingController = new BLEPairingController();
  
  pairingQueue = dispatch_queue_create("pairingQueue", NULL);
  
  void (^connectionHandler)(Anki::Networking::BLENetworkStream*) = ^(Anki::Networking::BLENetworkStream* stream) {
    dispatch_async(dispatch_get_main_queue(), ^() {
      [self onConnected:(Anki::Networking::INetworkStream*)stream];
    });
  };
  
  void (^disconnectionHandler)(Anki::Networking::BLENetworkStream*) = ^(Anki::Networking::BLENetworkStream* stream) {
    dispatch_async(dispatch_get_main_queue(), ^() {
      [self onDisconnected:(Anki::Networking::INetworkStream*)stream];
    });
  };
  
  pairingController->OnBLEConnectedEvent().SubscribeForever(connectionHandler);
  pairingController->OnBLEDisconnectedEvent().SubscribeForever(disconnectionHandler);
  pairingController->StartAdvertising();
}

- (void) onDisconnected: (Anki::Networking::INetworkStream*)stream {
  pinHandle = nullptr;
  wifiHandle = nullptr;
  
  securePairing->StopPairing();
}

- (void) onConnected: (Anki::Networking::INetworkStream*)stream {
  NSLog(@"Connected to BLE central!");
  if(securePairing == nullptr) {
    NSLog(@"Creating a new pairing object");
    securePairing = std::unique_ptr<Anki::Networking::SecurePairing>(new Anki::Networking::SecurePairing(stream, ev_default_loop(0)));
  }
  
  dispatch_async(pairingQueue, ^(void) {
    void (^wifiHandler)(std::string, std::string) = ^(std::string ssid, std::string pw) {
      NEHotspotConfiguration* wifi = [[NEHotspotConfiguration alloc] initWithSSID:[NSString stringWithUTF8String:ssid.c_str()] passphrase:[NSString stringWithUTF8String:pw.c_str()] isWEP:false];
      NSLog(@"Connecting to : [%s], [%s]", ssid.c_str(), pw.c_str());
      [[NEHotspotConfigurationManager sharedManager] applyConfiguration:wifi completionHandler:nullptr];
    };

    void (^pinHandler)(std::string) = ^(std::string pin) {
      dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"Updating pin label");
        _pinLabel.text = [NSString stringWithUTF8String:pin.c_str()];
      });
    };

    pinHandle = securePairing->OnUpdatedPinEvent().ScopedSubscribe(pinHandler);
    wifiHandle = securePairing->OnReceivedWifiCredentialsEvent().ScopedSubscribe(wifiHandler);
    
    // Initiate pairing process
    securePairing->BeginPairing();
  });
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

@end
