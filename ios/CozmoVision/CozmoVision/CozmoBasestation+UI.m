//
//  CozmoBasestation+UI.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/16/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "CozmoBasestation+UI.h"

@implementation CozmoBasestation (UI)

- (NSString*)basestationStateString
{
  NSString* stateStr;

  switch (self.runState) {
    case CozmoBasestationRunStateNone:
      stateStr = @"None";
      break;

    case CozmoBasestationRunStateCommsReady:
      stateStr = @"Searching";
      break;

    case CozmoBasestationRunStateRobotConnected:
      stateStr = @"Connected";
      break;

    case CozmoBasestationRunStateRunning:
      stateStr = @"Running";
      break;

    default:
      break;
  }
  return stateStr;
}
@end
