//
//  AppDelegate.m
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 1/11/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import "AppDelegate.h"
#import "BLECentralMultiplexer.h"
#import "BLECozmoManager.h"
#import "ViewController.h"
#import "AppTableData.h"
#import "BLELog.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "BLECozmoRandom.h"
#include "aes.h"

@interface AppDelegate ()

@property (nonatomic, strong) dispatch_queue_t bleQueue;
@property (nonatomic, strong) BLECentralMultiplexer* centralMultiplexer;
@property (nonatomic, strong) BLECozmoManager* cozmoManager;

@end


@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  // Override point for customization after application launch.
  
  self.bleQueue = dispatch_queue_create("com.anki.BLECentralMultiplexer", DISPATCH_QUEUE_SERIAL);
  CBCentralManager* centralManager = [[CBCentralManager alloc] initWithDelegate:nil
                                                                          queue:self.bleQueue];
  self.centralMultiplexer = [[BLECentralMultiplexer alloc] initWithCentralManager:centralManager queue:self.bleQueue];
  
  _cozmoManager = [[BLECozmoManager alloc] initWithCentralMultiplexer:self.centralMultiplexer
                                                                    queue:self.bleQueue];
  [_cozmoManager setServiceDelegate:self];
  
  _viewController = (ViewController*)  self.window.rootViewController;
  
  // Testing out aes implementation
  static constexpr uint32_t kKeySize = 16;
  uint8_t key[kKeySize] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  uint8_t message[kKeySize] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  AES128_ECB_encrypt(message, key, message);
  NSMutableString *string = [NSMutableString stringWithCapacity:kKeySize*2];
  for (NSInteger idx = 0; idx < kKeySize; ++idx) {
    [string appendFormat:@"%02x", (message)[idx]];
  }
  BLELogDebug("AppDelegate.encryptedMessage","0x%s", [string UTF8String]);
  AES128_ECB_decrypt(message, key, message);
  
  return YES;
}

- (void) startDiscovering {
  __weak __typeof(_cozmoManager) weakManager = _cozmoManager;
  dispatch_async(_bleQueue, ^{
    [weakManager discoverVehicles];
  });
}

- (void) stopDiscovering {
  __weak __typeof(_cozmoManager) weakManager = _cozmoManager;
  dispatch_async(_bleQueue, ^{
    [weakManager stopDiscoveringVehicles];
  });
}

- (void) doConnection:(UInt64)mfgID {
  __weak __typeof(_cozmoManager) weakManager = _cozmoManager;
  dispatch_async(_bleQueue, ^{
    [weakManager connectToVehicleByMfgID: mfgID];
    [weakManager stopDiscoveringVehicles];
  });
}

- (void) doDisconnection:(UInt64)mfgID {
  __weak __typeof(_cozmoManager) weakManager = _cozmoManager;
  dispatch_async(_bleQueue, ^{
    [weakManager disconnectVehicleWithMfgId:mfgID sendDisconnectMessages:YES];
  });
}

static uint32_t GetIP(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth)
{
  uint32_t sum = 0;
  sum += first << 0;
  sum += second << 8;
  sum += third << 16;
  sum += fourth << 24;
  return sum;
}

static Anki::Cozmo::RobotInterface::AppConnectConfigFlags InitConnectFlags()
{
  Anki::Cozmo::RobotInterface::AppConnectConfigFlags configFlags{};
  configFlags.apDHCPLeaseTime = 0xFFFFffff;
  configFlags.beaconInterval = 0xFFFF;
  configFlags.rfMax_dBm = 0xFF;
  configFlags.channel = 1;
  configFlags.maxConnections = 3;
  configFlags.authMode = 3;
  configFlags.apMinMaxSupRate = 0xFF;
  configFlags.wifiFixedRate = 0xFF;
  configFlags.staDHCPMaxTry = 0xFF;
  return configFlags;
}



- (void) doSendTestLightsMessage:(UInt64)mfgID {
  __weak __typeof(_cozmoManager) weakManager = _cozmoManager;
  dispatch_async(_bleQueue, ^{
    Anki::Cozmo::RobotInterface::BackpackLights lightsData{};
    lightsData.lights[1].onColor = lightsData.lights[1].offColor = Anki::Cozmo::LED_ENC_BLU;
    lightsData.lights[1].onFrames = lightsData.lights[1].offFrames = 0x02;
    lightsData.lights[2].onColor = lightsData.lights[2].offColor = Anki::Cozmo::LED_ENC_RED;
    lightsData.lights[2].onFrames = lightsData.lights[2].offFrames = 0x02;
    lightsData.lights[3].onColor = lightsData.lights[3].offColor = Anki::Cozmo::LED_ENC_GRN;
    lightsData.lights[3].onFrames = lightsData.lights[3].offFrames = 0x02;
    
    lightsData.lights[1].offColor = lightsData.lights[2].offColor = lightsData.lights[3].offColor = Anki::Cozmo::LED_ENC_OFF;
    
    Anki::Cozmo::RobotInterface::EngineToRobot robotMsg{};
    robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_setBackpackLights;
    robotMsg.setBackpackLights = lightsData;
    
    [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                       error:nil
                                                   encrypted:NO];
  });
  
  static constexpr int kNumFlashSeconds = 1;
  // Queue up a message to send to turn off flashy lights
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(kNumFlashSeconds * NSEC_PER_SEC)), _bleQueue, ^{
    Anki::Cozmo::RobotInterface::EngineToRobot robotMsg{};
    robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_setBackpackLights;
    robotMsg.setBackpackLights = Anki::Cozmo::RobotInterface::BackpackLights{}; // Use an empty message to shut off lights
    
    [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                       error:nil
                                                   encrypted:NO];
  });
}

- (void) doConfigWifiMessage:(UInt64)mfgID ssid:(NSString*)ssid password:(NSString*)password {
  __weak __typeof(_cozmoManager) weakManager = _cozmoManager;
  dispatch_async(_bleQueue, ^{
    // First send the first half (or all if it's short enough) of new ssid
    static constexpr int kMaxStringMessageLength = 16;
    Anki::Cozmo::RobotInterface::AppConnectConfigString configString{};
    configString.id = Anki::Cozmo::RobotInterface::AP_AP_SSID_0;
    NSUInteger charsToCopy = ssid.length < kMaxStringMessageLength ? ssid.length : kMaxStringMessageLength;
    memcpy(configString.data, [ssid UTF8String], charsToCopy);
    
    Anki::Cozmo::RobotInterface::EngineToRobot robotMsg{};
    robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgString;
    robotMsg.appConCfgString = configString;
    [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                       error:nil
                                                   encrypted:NO];
    
    // Send 2nd half of string if needed
    configString = Anki::Cozmo::RobotInterface::AppConnectConfigString{};
    configString.id = Anki::Cozmo::RobotInterface::AP_AP_SSID_1;
    charsToCopy = ssid.length > kMaxStringMessageLength ? ssid.length - kMaxStringMessageLength : 0;
    
    if (charsToCopy > 0)
    {
      charsToCopy = charsToCopy > kMaxStringMessageLength ? kMaxStringMessageLength : charsToCopy;
      memcpy(configString.data, [ssid UTF8String] + kMaxStringMessageLength, charsToCopy);
      
      robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgString;
      robotMsg.appConCfgString = configString;
      [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                         error:nil
                                                     encrypted:NO];
    }
    
    // Set up password
    static constexpr int kMaxPasswordLength = 4 * kMaxStringMessageLength;
    NSUInteger numCharsForPass = password.length;
    if (numCharsForPass > kMaxPasswordLength)
    {
      numCharsForPass = kMaxPasswordLength;
      BLELogError("AppDelegate.doConfigWifiMessage","Password truncated to %d characters!", kMaxPasswordLength);
    }
    
    configString = Anki::Cozmo::RobotInterface::AppConnectConfigString{};
    configString.id = Anki::Cozmo::RobotInterface::AP_AP_PSK_0;
    charsToCopy = numCharsForPass < kMaxStringMessageLength ? numCharsForPass : kMaxStringMessageLength;
    memcpy(configString.data, [password UTF8String], charsToCopy);
    numCharsForPass -= charsToCopy;
    
    robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgString;
    robotMsg.appConCfgString = configString;
    [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                       error:nil
                                                   encrypted:NO];
    
    // Repeat for 2nd password chunk if needed
    if (numCharsForPass > 0)
    {
      configString = Anki::Cozmo::RobotInterface::AppConnectConfigString{};
      configString.id = Anki::Cozmo::RobotInterface::AP_AP_PSK_1;
      charsToCopy = numCharsForPass < kMaxStringMessageLength ? numCharsForPass : kMaxStringMessageLength;
      memcpy(configString.data, [password UTF8String] + kMaxStringMessageLength, charsToCopy);
      numCharsForPass -= charsToCopy;
      
      robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgString;
      robotMsg.appConCfgString = configString;
      [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                         error:nil
                                                     encrypted:NO];
    }
    
    // Repeat for 3rd password chunk if needed
    if (numCharsForPass > 0)
    {
      configString = Anki::Cozmo::RobotInterface::AppConnectConfigString{};
      configString.id = Anki::Cozmo::RobotInterface::AP_AP_PSK_2;
      charsToCopy = numCharsForPass < kMaxStringMessageLength ? numCharsForPass : kMaxStringMessageLength;
      memcpy(configString.data, [password UTF8String] + (kMaxStringMessageLength * 2), charsToCopy);
      numCharsForPass -= charsToCopy;
      
      robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgString;
      robotMsg.appConCfgString = configString;
      [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                         error:nil
                                                     encrypted:NO];
    }
    
    // Repeat for 4th password chunk if needed
    if (numCharsForPass > 0)
    {
      configString = Anki::Cozmo::RobotInterface::AppConnectConfigString{};
      configString.id = Anki::Cozmo::RobotInterface::AP_AP_PSK_3;
      charsToCopy = numCharsForPass < kMaxStringMessageLength ? numCharsForPass : kMaxStringMessageLength;
      memcpy(configString.data, [password UTF8String] + (kMaxStringMessageLength * 3), charsToCopy);
      numCharsForPass -= charsToCopy;
      
      robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgString;
      robotMsg.appConCfgString = configString;
      [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                         error:nil
                                                     encrypted:NO];
    }
    
    // Set up config flags #1
    Anki::Cozmo::RobotInterface::AppConnectConfigFlags configFlags = InitConnectFlags();
    configFlags.apFlags = Anki::Cozmo::RobotInterface::AP_PHY_G | Anki::Cozmo::RobotInterface::AP_SOFTAP | Anki::Cozmo::RobotInterface::AP_APPLY_SETTINGS;
    configFlags.beaconInterval = 100;
    configFlags.channel = 5;
    configFlags.authMode = 0; // 0 for off 3 for  WPA2_PSK
    
    robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgFlags;
    robotMsg.appConCfgFlags = configFlags;
    [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                       error:nil
                                                   encrypted:NO];
    
    // Set up config flags #2, bring down dhcp
    configFlags = InitConnectFlags();
    configFlags.apFlags = Anki::Cozmo::RobotInterface::AP_SOFTAP | Anki::Cozmo::RobotInterface::AP_AP_DHCP_STOP;
    
    robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgFlags;
    robotMsg.appConCfgFlags = configFlags;
    [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                       error:nil
                                                   encrypted:NO];
    
    // Set up IP Info
    Anki::Cozmo::RobotInterface::AppConnectConfigIPInfo configIPInfo{};
    configIPInfo.robotIp = GetIP(172,31,1,1);
    configIPInfo.robotNetmask = GetIP(255,255,255,0);
    configIPInfo.robotGateway = GetIP(172,31,1,1);
    configIPInfo.ifId = 1; // SoftAP Interface
    
    robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgIPInfo;
    robotMsg.appConCfgIPInfo = configIPInfo;
    [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                       error:nil
                                                   encrypted:NO];
    
    // Set up config flags #3, bring back up dhcp
    configFlags = InitConnectFlags();
    configFlags.apFlags = Anki::Cozmo::RobotInterface::AP_SOFTAP | Anki::Cozmo::RobotInterface::AP_AP_DHCP_START;
    
    robotMsg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_appConCfgFlags;
    robotMsg.appConCfgFlags = configFlags;
    [[weakManager connectionForMfgID:mfgID] writeMessageData:[[NSData alloc] initWithBytes:robotMsg.GetBuffer() length:robotMsg.Size()]
                                                       error:nil
                                                   encrypted:NO];
  });
}


#pragma mark BLECozmoServiceDelegate implementation
-(void)vehicleManager:(BLECozmoManager*)manager vehicleDidAppear:(BLECozmoConnection *)connection {
  [[_viewController getAppTableData] addCozmo:[NSString stringWithFormat:@"0x%llx", connection.mfgID]];
}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidDisappear:(BLECozmoConnection *)connection {
  [[_viewController getAppTableData] removeCozmo:[NSString stringWithFormat:@"0x%llx", connection.mfgID]];
}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidConnect:(BLECozmoConnection *)connection {

}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidDisconnect:(BLECozmoConnection *)connection {

}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidUpdateAdvertisement:(BLECozmoConnection *)connection {

}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidUpdateProximity:(BLECozmoConnection *)connection {
  
}

-(void)vehicleManager:(BLECozmoManager *)manager vehicle:(BLECozmoConnection *)connection didReceiveMessage:(NSData *)message {

}



#pragma mark  Simple App Stuff
- (void)applicationWillResignActive:(UIApplication *)application {
  // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
  // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
  // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
  // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
  if (self->bgTask == UIBackgroundTaskInvalid) {
    bgTask = [application beginBackgroundTaskWithExpirationHandler: ^{
      dispatch_async(dispatch_get_main_queue(), ^{
        [application endBackgroundTask:self->bgTask];
        self->bgTask = UIBackgroundTaskInvalid;
      });
    }];
  }
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
  // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
  // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application {
  // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
