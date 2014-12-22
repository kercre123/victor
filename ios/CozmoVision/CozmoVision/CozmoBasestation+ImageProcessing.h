//
//  CozmoBasestation+ImageProcessing.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/12/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "CozmoBasestation.h"
#import <opencv2/opencv.hpp>

@interface CozmoBasestation (ImageProcessing)

- (UIImage*)UIImageFromCVMat:(cv::Mat)cvMat;

@end
