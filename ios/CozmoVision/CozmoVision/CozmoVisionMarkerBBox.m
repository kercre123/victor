//
//  CozmoVisionMarkerBBox.m
//  CozmoVision
//
//  Created by Andrew Stein on 1/21/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#import "CozmoVisionMarkerBBox.h"

@implementation CozmoVisionMarkerBBox

@synthesize markerType;
@synthesize boundingBox;

-(void)setBoundingBoxFromCornersWithXupperLeft:(Float32)xUL
                                    YupperLeft:(Float32)yUL
                                    XlowerLeft:(Float32)xLL
                                    YlowerLeft:(Float32)yLL
                                   XupperRight:(Float32)xUR
                                   YupperRight:(Float32)yUR
                                   XlowerRight:(Float32)xLR
                                   YlowerRight:(Float32)yLR
{
  Float32 xMin = fmin(fmin(xLL, xLR),
                      fmin(xUL, xUR));
  Float32 yMin = fmin(fmin(yLL, yLR),
                      fmin(yUL, yUR));
  
  Float32 xMax = fmax(fmax(xLL, xLR),
                      fmax(xUL, xUR));
  Float32 yMax = fmax(fmax(yLL, yLR),
                      fmax(yUL, yUR));
  
  self.boundingBox = CGRectMake(xMin, yMin, xMax-xMin, yMax-yMin);
}

@end