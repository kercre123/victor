//
//  AppTableData.m
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/13/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import "AppTableData.h"

@interface AppTableData () <UITableViewDataSource, UITableViewDelegate>

@property (strong, nonatomic) NSMutableSet* cozmoNames;
@property (strong, nonatomic) NSString* selectedName;
@property (weak, nonatomic) UITableView* idListTable;

@end

@implementation AppTableData

-(id)initWithTableView:(UITableView*)tableView
{
  _cozmoNames = [NSMutableSet set];
  _selectedName = nil;
  _idListTable = tableView;
  return self;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"MyIdentifier"];
  if (cell == nil) {
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"MyIdentifier"];
    cell.selectionStyle = UITableViewCellSelectionStyleDefault;
  }
  NSArray* allObjects = [_cozmoNames allObjects];
  cell.textLabel.text = (NSString*)[allObjects objectAtIndex:indexPath.row];
  return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
  return [_cozmoNames count];
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

-(void)addCozmo:(NSString*)name
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [_cozmoNames addObject:name];
    [_idListTable reloadData];
  });
}

-(void)removeCozmo:(NSString*)name
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [_cozmoNames removeObject:name];
    [_idListTable reloadData];
  });
}

-(void)clearList
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [_cozmoNames removeAllObjects];
    [_idListTable reloadData];
  });
}

-(NSString*)getSelectedCozmo
{
  NSIndexPath* path = [_idListTable indexPathForSelectedRow];
  if (!path) return nil;
  
  UITableViewCell* cell = [_idListTable cellForRowAtIndexPath:path];
  if (!cell) return nil;
  
  return cell.textLabel.text;
}

@end