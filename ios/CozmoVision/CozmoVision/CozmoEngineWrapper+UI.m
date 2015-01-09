//
//  CozmoEngineWrapper+UI.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/16/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "CozmoEngineWrapper+UI.h"

@implementation CozmoEngineWrapper (UI)

- (NSString*)engineStateString
{
  NSString* stateStr;

  switch (self.runState) {
    case CozmoEngineRunStateNone:
      stateStr = @"None";
      break;

    case CozmoEngineRunStateCommsReady:
      stateStr = @"Searching";
      break;

    case CozmoEngineRunStateRobotConnected:
      stateStr = @"Connected";
      break;

    case CozmoEngineRunStateRunning:
      stateStr = @"Running";
      break;

    default:
      break;
  }
  return stateStr;
}
@end
