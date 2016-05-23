//
//  DASClientInfo.h
//  DAS
//
//  Created by tom on 1/3/13.
//  Copyright (c) 2013 Anki. All rights reserved.
//

#import <Foundation/Foundation.h>

extern NSString * const DASAppRunKey;
extern NSString * const DASGameIDKey;
extern NSString * const DASUserEmailKey;
extern NSString * const DASUserIDKey;

@interface DASClientInfo : NSObject

// Events Exposed
- (void) eventsMainStart:(const char *)dasUser appVersion:(const char *)appVersion;
- (void) eventsMainStart:(const char *)dasUser appVersion:(const char *)appVersion product:(const char *)product;

// Client Info Exposed
+ (NSString *) deviceID;
+ (NSString *) model;

// first 12 digits of vendorIdentifier
+ (NSString *)supportVendorID;

+ (instancetype)sharedInfo;

// DAS identifier for the currently launched app instance
@property (nonatomic, readonly, copy) NSString *appRunID;

// DAS identifier for the previously launched app instance, if one exists
@property (nonatomic, readonly, copy) NSString *lastAppRunID;

// Set current $game identifier
@property (nonatomic, copy) NSString *gameID;

// Set current $user identifier
@property (nonatomic, copy) NSString *userID;

@end

extern NSString* DASPeriodicNotification;
