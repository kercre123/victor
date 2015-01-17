//
//  CozmoOperator.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/11/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface CozmoOperator : NSObject

@property (readonly) BOOL isConnected;

+ (instancetype)operatorWithAdvertisingtHostIPAddress:(NSString*)address
                                         withDeviceID:(int)deviceID;

// Connection Methods
- (void)registerToAvertisingServiceWithIPAddress:(NSString*)ipAddress
                                    withDeviceID:(int)deviceID;
//- (void)disconnect;


- (void)update;


// Drive Cozmo Methods
/*!
 * @discussion Send Wheel movment command.
 * @param Angle in degrees - 0° is arch tan2
 * @param Magnitude ratio [1.0(forward) - 0.0(stop)]
 * @return void
 */
- (void)sendWheelCommandWithAngleInDegrees:(float)angle magnitude:(float)magnitude;

// This is for debug
- (void)sendWheelCommandWithLeftSpeed:(float)left right:(float)right;

// This is not ready yet just use simple method
///*!
// * @discussion Send head angle command.
// * @param Angle using a ratio [0.0(down) - 1.0(up)]
// * @param Acceleration movement speed ratio [0.0(no acceleration) - 1.0(full acceleration)]
// * @param Max movement speed ratio [0.0(no speed) - 1.0(full speed)]
// * @return void
// */
//- (void)sendHeadCommandWithAngleRatio:(float)angle accelerationSpeedRatio:(float)acceleration maxSpeedRatio:(float)maxSpeed;

/*!
 * @discussion Send head angle command using default acceleration & max speed.
 * @param Angle using a ratio [0.0(down) - 1.0(up)]
 * @return void
 */
- (void)sendHeadCommandWithAngleRatio:(float)angle;

// This is not ready yet just use simple method
///*!
// * @discussion Send lift height command
// * @param Height ratio [0.0(down) - 1.0(up)]
// * @param Acceleration movement speed ratio [0.0(no acceleration) - 1.0(full acceleration)]
// * @param Max movement speed ratio [0.0(no speed) - 1.0(full speed)]
// * @return void
// */
//- (void)sendLiftCommandWithHeightRatio:(float)height accelerationSpeedRatio:(float)acceleration maxSpeedRatio:(float)maxSpeed;

/*!
 * @discussion Send lift height command with default acceleration & max speed.
 * @param Height ratio [0.0(down) - 1.0(up)]
 * @return void
 */
- (void)sendLiftCommandWithHeightRatio:(float)height;


/*!
 * @discussion Send Stop All Motors command.
 * @return void
 */
- (void)sendStopAllMotorsCommand;


/*!
 * @discussion Send Pick or Place Object command
 * @param ObjectID
 * @return void
 */
-(void)sendPickOrPlaceObject:(NSNumber*)objectID;

-(void)sendPlaceObjectOnGroundHere;

-(void)sendEnableFaceTracking:(BOOL)enable;

- (void)sendAnimationWithName:(NSString*)animationName;

- (void)sendCameraResolution:(BOOL)isHigh;

- (void)sendEnableRobotImageStreaming:(BOOL)enable;

@end
