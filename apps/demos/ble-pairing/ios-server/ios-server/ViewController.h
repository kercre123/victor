//
//  ViewController.h
//  ios-server
//
//  Created by Paul Aluri on 1/9/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "networking/INetworkStream.h"
#include "networking/securePairing.h"

@interface ViewController : UIViewController {
  dispatch_queue_t pairingQueue;
}

@end

