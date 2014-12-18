//
//  GameSelectViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/15/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "GameSelectViewController.h"
#import "CozmoBasestation.h"
#import "CozmoBasestation+UI.h"


@interface GameSelectViewController ()

@property (weak, nonatomic) IBOutlet UIBarButtonItem *basestationStateBarItem;

@property (weak, nonatomic) CozmoBasestation *_basestation;

@end

@implementation GameSelectViewController

- (void)viewDidLoad
{
  [super viewDidLoad];

  self._basestation = [CozmoBasestation defaultBasestation];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];

  [self.basestationStateBarItem setTitle:[self._basestation basestationStateString]];
  [self._basestation addObserver:self forKeyPath:@"runState" options:NSKeyValueObservingOptionNew context:nil];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [self._basestation removeObserver:self forKeyPath:@"runState" context:nil];
  [super viewWillDisappear:animated];
}


#pragma mark - KVO Methods

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
  if ([object isEqual:self._basestation]) {

    if ([keyPath isEqualToString:@"runState"]) {
      [self.basestationStateBarItem setTitle:[self._basestation basestationStateString]];
    }
  }
}

@end
