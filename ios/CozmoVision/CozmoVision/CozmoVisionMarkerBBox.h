//
//  CozmoVisionMarkerBBox.h
//  CozmoVision
//
//    Wrapper for the rectangular bounding box of a VisionMarker from CozmoEngine.
//
//  Created by Andrew Stein on 1/21/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface CozmoVisionMarkerBBox : NSObject

@property NSInteger    markerType;
@property CGRect       boundingBox;

-(void)setBoundingBoxFromCornersWithXupperLeft:(Float32)xUL
                                    YupperLeft:(Float32)yUL
                                    XlowerLeft:(Float32)xLL
                                    YlowerLeft:(Float32)yLL
                                   XupperRight:(Float32)xUR
                                   YupperRight:(Float32)yUR
                                   XlowerRight:(Float32)xLR
                                   YlowerRight:(Float32)yLR;

@end
