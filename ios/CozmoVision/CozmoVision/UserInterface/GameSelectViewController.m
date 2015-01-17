//
//  GameSelectViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/15/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "GameSelectViewController.h"
#import "CozmoEngineWrapper.h"
#import "CozmoEngineWrapper+UI.h"
#import "CozmoOperator.h"
#import "RobotAnimationSelectTableViewController.h"


@interface GameSelectViewController ()

@property (weak, nonatomic) IBOutlet UIBarButtonItem *engineStateBarItem;

@property (weak, nonatomic) CozmoEngineWrapper *cozmoEngineWrapper;

@end

@implementation GameSelectViewController

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.cozmoEngineWrapper = [CozmoEngineWrapper defaultEngine];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];

  [self.engineStateBarItem setTitle:[self.cozmoEngineWrapper engineStateString]];
  [self.cozmoEngineWrapper addObserver:self forKeyPath:@"runState" options:NSKeyValueObservingOptionNew context:nil];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [self.cozmoEngineWrapper removeObserver:self forKeyPath:@"runState" context:nil];
  [super viewWillDisappear:animated];
}


#pragma mark - KVO Methods

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
  if ([object isEqual:self.cozmoEngineWrapper]) {

    if ([keyPath isEqualToString:@"runState"]) {
      [self.engineStateBarItem setTitle:[self.cozmoEngineWrapper engineStateString]];
    }
  }
}


#pragma mark - Action Mehothds

- (IBAction)handleAnimationButtonPress:(id)sender
{
  RobotAnimationSelectTableViewController *vc = [RobotAnimationSelectTableViewController new];
  [vc setDidSelectAnimationAction:^(NSString *name) {
    [[[CozmoEngineWrapper defaultEngine] cozmoOperator] sendAnimationWithName:name];
  }];
  [self.navigationController pushViewController:vc animated:YES];
}

@end
