//
//  AppTableData.h
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/13/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface AppTableData : UIViewController <UITableViewDataSource,UITableViewDelegate>

-(id)initWithTableView:(UITableView*)tableView;
-(void)addCozmo:(NSString*)name;
-(void)removeCozmo:(NSString*)name;
-(NSString*)getSelectedCozmo;
-(void)clearList;

@end