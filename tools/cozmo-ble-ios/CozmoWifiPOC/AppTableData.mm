//
//  AppTableData.m
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/13/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import "AppTableData.h"
#import "ViewController.h"

@interface AppTableData () <UITableViewDataSource, UITableViewDelegate>

@property (weak, nonatomic) ViewController* viewController;

@end

@implementation AppTableData

-(id)initWithViewController:(ViewController*)viewController
{
  _viewController = viewController;
  return self;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"MyIdentifier"];
  if (cell == nil) {
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"MyIdentifier"];
    cell.selectionStyle = UITableViewCellSelectionStyleDefault;
  }
  
  cell.textLabel.text = [_viewController getStringForRow:indexPath.row];
  return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
  return [_viewController getNumRows];
}

// Allow for deselecting the selected row
- (NSIndexPath *)tableView:(UITableView *)tableView
  willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
  UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
  if ([cell isSelected]) {
    [tableView deselectRowAtIndexPath:indexPath animated:NO];
    return nil;
  }
  
  return indexPath;
}

@end