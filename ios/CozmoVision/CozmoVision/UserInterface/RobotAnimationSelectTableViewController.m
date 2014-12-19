//
//  RobotAnimationSelectTableViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/18/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "RobotAnimationSelectTableViewController.h"
#import <anki/common/basestation/platformPathManager_iOS_interface.h>

@interface RobotAnimationSelectTableViewController ()
//@property (strong, nonatomic) NSArray* animationTitles;
@end

// Only generate once
const NSArray* _animationTitles = nil;

@implementation RobotAnimationSelectTableViewController

const NSString* kCellReuseIdentifier = @"kCellReuseIdentifier";

- (id)initWithCoder:(NSCoder *)aDecoder
{
  if (!(self = [super initWithCoder:aDecoder]))
    return self;

  [self commonInit];
  
  return self;
}

- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
  if (!(self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]))
    return self;

  [self commonInit];

  return self;
}

- (void)commonInit
{
  // Read Animation Titles
  if (!_animationTitles) {
    NSString* animationFile = [[PlatformPathManager_iOS getPathWithScope:PlatformPathManager_iOS_Scope_Animation] stringByAppendingString:@"animations.json"];
    if (animationFile) {
      NSDictionary* animationJSONData = [NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:animationFile] options:0 error:nil];
      if (animationJSONData) {
        NSArray *allTitles = [animationJSONData allKeys];
        _animationTitles = [allTitles sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
      }
    }
  }

  [self.tableView registerClass:[UITableViewCell class] forCellReuseIdentifier:(NSString*)kCellReuseIdentifier];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return _animationTitles.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell* cell = [self.tableView dequeueReusableCellWithIdentifier:(NSString*)kCellReuseIdentifier forIndexPath:indexPath];
  cell.textLabel.text = _animationTitles[indexPath.row];

  return cell;
}


- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  
  if (self.didSelectAnimationAction) {
    NSString* selectedTitle = _animationTitles[indexPath.row];
    self.didSelectAnimationAction(selectedTitle);
  }
}

@end
