//
//  ViewController.h
//  ios-server
//
//  Created by Paul Aluri on 1/9/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "networking/INetworkStream.h"
#include "networking/SecurePairing.h"

@interface ViewController : UIViewController {
  std::unique_ptr<Anki::Networking::SecurePairing> securePairing;
  dispatch_queue_t pairingQueue;
  Signal::SmartHandle pinHandle;
  Signal::SmartHandle wifiHandle;
}

- (void) onConnected: (Anki::Networking::INetworkStream*)stream;
- (void) onDisconnected: (Anki::Networking::INetworkStream*)stream;

@end

