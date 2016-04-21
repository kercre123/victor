//
//  AppDelegate.h
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 1/11/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import "BLECozmoServiceDelegate.h"

@class BLECentralMultiplexer;
@class ViewController;

@interface AppDelegate : UIResponder <UIApplicationDelegate, BLECozmoServiceDelegate>
{
  UIBackgroundTaskIdentifier bgTask;
}

@property (strong, nonatomic) UIWindow *window;
@property BOOL shouldInstallProfile;
@property (nonatomic, strong, readonly) BLECentralMultiplexer* centralMultiplexer;
@property (strong, nonatomic) ViewController  *viewController;

- (void) startDiscovering;
- (void) stopDiscovering;
- (void) doConnection:(UInt64)mfgID;
- (void) doDisconnection:(UInt64)mfgID;
- (void) doSendTestLightsMessage:(UInt64)mfgID;
- (void) doConfigWifiMessage:(UInt64)mfgID ssid:(NSString*)ssid password:(NSString*)password;

@end