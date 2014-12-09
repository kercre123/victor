//
//  CozmoBasestation.h
//  CozmoVision
//
//  Created by Andrew Stein on 12/5/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

#include "anki/cozmo/basestation/basestation.h"
//#include "anki/messaging/basestation/IComms.h"
#include "anki/cozmo/basestation/robotComms.h"
#include "anki/cozmo/basestation/uiTcpComms.h"

#define FRAME_RATE 40.0 // Ticks per second
#define PERIOD ( NSEC_PER_SEC / (double)FRAME_RATE )

@interface CozmoBasestation : NSObject  {

  NSTimer*                        _heartbeatTimer;
  Anki::Cozmo::BasestationMain*   _basestation;
  Anki::Cozmo::RobotComms*        _robotComms;
  Anki::Cozmo::UiTCPComms*        _uiComms;

}

  @property UIImageView*          _imageViewer;


//-(void)initializeGame;
-(void)gameHeartbeat:(NSTimer*)timer;

@end
