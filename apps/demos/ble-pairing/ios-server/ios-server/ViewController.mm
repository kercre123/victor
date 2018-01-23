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
#include "networking/BLEPairingController.h"
#include "networking/SecurePairing.h"
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
  
  void (^connectionHandler)(Anki::Networking::BLENetworkStream*) = ^(Anki::Networking::BLENetworkStream* stream) {
    [self onConnected:(Anki::Networking::INetworkStream*)stream];
  };
  
  pairingController->OnBLEConnectedEvent().SubscribeForever(connectionHandler);
  pairingController->StartAdvertising();
}

- (void) onConnected: (Anki::Networking::INetworkStream*)stream {
  Anki::Networking::SecurePairing* securePairing = new Anki::Networking::SecurePairing(stream);
  
  _pinLabel.text = [NSString stringWithUTF8String:(const char*)securePairing->GetPin()];
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

@end
