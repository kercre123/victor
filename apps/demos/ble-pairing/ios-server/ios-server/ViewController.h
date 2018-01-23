//
//  ViewController.h
//  ios-server
//
//  Created by Paul Aluri on 1/9/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "networking/INetworkStream.h"

@interface ViewController : UIViewController

- (void) onConnected: (Anki::Networking::INetworkStream*)stream;

@end

