//
//  ObservedObjectSelector.m
//  CozmoVision
//
//  Created by Andrew Stein on 12/17/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "ObservedObjectSelector.h"


@interface ObservedObjectSelector ()

//@property (strong, nonatomic) UITapGestureRecognizer* tapGesture;

//- (void)handleTapGesture:(UITapGestureRecognizer*)gesture;

@end


@implementation ObservedObjectSelector

@synthesize observedObjects;

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
}
*/

/*
- (instancetype)init
{
  if (!(self = [super init]))
    return self;
  
  [self commonInit];
  
  return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
  if (!(self = [super initWithFrame:frame]))
    return self;
  
  [self commonInit];
  
  return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
  if (!(self = [super initWithCoder:aDecoder]))
    return self;
  
  [self commonInit];
  
  return self;
}

- (void)commonInit
{
  self.tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
  self.tapGesture.numberOfTapsRequired = 2; // require double-tap to select
  self.tapGesture.cancelsTouchesInView = NO; // pass through non-double-taps
  self.tapGesture.delegate = self;
  [self addGestureRecognizer:self.tapGesture];
  
  
  
//  self.userInteractionEnabled = NO;
}


- (void)handleTapGesture:(UITapGestureRecognizer*)gesture
{
  CGPoint point = [gesture locationInView:self];
  
  // Scale the point into image pixel coordinates
  CGPoint scaledPoint = CGPointMake(point.x / self.bounds.size.width * 320,
                                    point.y / self.bounds.size.height * 240);
  
  for(CozmoObsObjectBBox* obsObject in self.observedObjects)
  {
    if(CGRectContainsPoint(obsObject.boundingBox, scaledPoint)) {
      NSLog(@"Selected Object %d\n", obsObject.objectID);
      break;
    }
  }
}

*/

-(NSNumber *)checkForSelectedObject:(CGPoint)normalizedPoint
{
  // Scale the point into image pixel coordinates
  // TODO: Don't hardcode the raw image size
  CGPoint scaledPoint = CGPointMake(normalizedPoint.x * 320,
                                    normalizedPoint.y * 240);
  
  for(CozmoObsObjectBBox* obsObject in self.observedObjects)
  {
    if(CGRectContainsPoint(obsObject.boundingBox, scaledPoint)) {
      return [NSNumber numberWithInteger:obsObject.objectID];
    }
  }
  
  return nil;
}

@end


