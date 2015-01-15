//
//  CozmoEngineWrapper+ImageProcessing.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/12/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "CozmoEngineWrapper+ImageProcessing.h"
// Coretech-Vision includes
#import <anki/vision/basestation/image_impl.h>

@implementation CozmoEngineWrapper (ImageProcessing)

- (UIImage*)UIImageFromCVMat:(cv::Mat)cvMat
{
  NSData *data = [NSData dataWithBytes:cvMat.data length:cvMat.elemSize()*cvMat.total()];
  CGColorSpaceRef colorSpace;

  if (cvMat.elemSize() == 1) {
    colorSpace = CGColorSpaceCreateDeviceGray();
  } else {
    colorSpace = CGColorSpaceCreateDeviceRGB();
  }

  CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)data);

  // Creating CGImage from cv::Mat
  CGImageRef imageRef = CGImageCreate(cvMat.cols,                                 //width
                                      cvMat.rows,                                 //height
                                      8,                                          //bits per component
                                      8 * cvMat.elemSize(),                       //bits per pixel
                                      cvMat.step[0],                              //bytesPerRow
                                      colorSpace,                                 //colorspace
                                      kCGImageAlphaNone|kCGBitmapByteOrderDefault,// bitmap info
                                      provider,                                   //CGDataProviderRef
                                      NULL,                                       //decode
                                      false,                                      //should interpolate
                                      kCGRenderingIntentDefault                   //intent
                                      );


  // Getting UIImage from CGImage
  UIImage *finalImage = [UIImage imageWithCGImage:imageRef];
  CGImageRelease(imageRef);
  CGDataProviderRelease(provider);
  CGColorSpaceRelease(colorSpace);

  return finalImage;
}

@end
