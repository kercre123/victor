//
//  ObservedObjectSelector.h
//  CozmoVision
//
//  Created by Andrew Stein on 12/17/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "CozmoObsObjectBBox.h"

@interface ObservedObjectSelector : NSObject

@property NSArray* observedObjects;

-(NSNumber*)checkForSelectedObject:(CGPoint)normalizedPoint;

@end
