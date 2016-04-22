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
@class BLECozmoConnection;

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
- (void) doConnection:(BLECozmoConnection*)connection;
- (void) doDisconnection:(BLECozmoConnection*)connection;
- (void) doSendTestLightsMessage:(BLECozmoConnection*)connection;
- (void) doConfigWifiMessage:(BLECozmoConnection*)connection ssid:(NSString*)ssid password:(NSString*)password;

@end