//
//  RobotAnimationSelectTableViewController.h
//  CozmoVision
//
//  Created by Jordan Rivas on 12/18/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface RobotAnimationSelectTableViewController : UITableViewController
@property (copy, nonatomic) void (^didSelectAnimationAction)(NSString* title);
@end
