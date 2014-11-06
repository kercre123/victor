//
//  BLEManager.h
//  BLEManager
//
//  Created by Mark Pauley on 9/22/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//
#import <Foundation/Foundation.h>

/*!
 * A BLEManagerNotification is dispatched to the default notification center
 * The info dictionary of the notification will be one of the following:
 * @see BLEManagerKeyPowerIsOn
 * @see BLEManagerKeyBLESupported
 * @see BLEManagerKeyBLEUnauthorized
 */
extern NSString* const BLEManagerNotificationName;

/*!
 * Value is an NSNumber (BOOL)
 */
extern NSString* const BLEManagerKeyPowerIsOn;

/*!
 * Value is an NSNumber (BOOL)
 */
extern NSString* const BLEManagerKeyBLEUnsupported;

/*!
 * Value is an NSNumber (BOOL)
 */
extern NSString* const BLEManagerKeyBLEUnauthorized;

/*!
 * Forces initialization of the CoreBluetooth library
 * @discussion Call BLEManagerInitialize sometime after registering for the BLEManagerNotificationName notification.
 */
void BLEManagerInitialize();

