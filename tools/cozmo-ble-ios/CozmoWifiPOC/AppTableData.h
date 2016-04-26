//
//  AppTableData.h
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/13/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class ViewController;

@interface AppTableData : UIViewController <UITableViewDataSource,UITableViewDelegate>

-(id)initWithViewController:(ViewController*)viewController;

@end