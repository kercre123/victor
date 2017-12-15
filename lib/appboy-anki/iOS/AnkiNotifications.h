//
//  AnkiNotifications.h
//  AnkiNotifications
//
//  Created by Andrew Hilvers on 9/28/17.
//  Copyright © 2017 Andrew Hilvers. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#define NOTIFICATION_OPENED @"OnAnkiNotificationOpened%@"
#define NOTIFICATION_RECEIVED_FOREGROUND @"OnAnkiNotificationReceivedForeground%@"
#define NOTIFICATION_RECEIVED_BACKGROUND @"OnAnkiNotificationReceivedBackground%@"

@interface AnkiNotifications : NSObject

+ (AnkiNotifications*) sharedInstance;

- (void)application:(UIApplication* )application didFinishLaunchingWithOptions:(NSDictionary* )launchOptions
apiKey:(NSString* )apiKey pushEnabled:(BOOL)pushEnabled;

- (void)application:(UIApplication *)application
didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken;

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo
fetchCompletionHandler:(void (^)(UIBackgroundFetchResult result))completionHandler;

- (void)applicationDidBecomeActive:(UIApplication *)application;

- (void)changeAppBoyUserId:(NSString*)userId;

- (BOOL)addAppBoyAlias:(NSString *)alias withLabel:(NSString *)label;

- (void)setAppBoyUserEmail:(NSString*)email;

- (void)setAppBoyUserDOB:(NSDate*)DOB;

- (void) logWithFormatting:(NSString*)log;

- (NSString*) convertDictionaryToJSON:(NSDictionary*)dictionary;

- (void) setShowNotificationInForeground:(BOOL)shouldShow;

- (BOOL) shouldShowNotificationInForeground;

- (void) setCustomUserAttribute:(NSString*) attributeKey withValue:(NSString*)value;

- (void) setRequestProcessingPolicy:(BOOL)automatic;

@end
