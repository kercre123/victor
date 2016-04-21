//
//  ViewController.h
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 1/11/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import <UIKit/UIKit.h>

@class AppTableData;

@interface ViewController : UIViewController

// MARK: Properties
@property (weak, nonatomic) IBOutlet UITextField *ssidTextField;
@property (weak, nonatomic) IBOutlet UITextField *passwordTextField;
@property (weak, nonatomic) IBOutlet UILabel *connectionDetailsLabel;
@property (weak, nonatomic) IBOutlet UITableView *idListTable;

// MARK: Actions
- (IBAction)startDiscoverAction:(id)sender;
- (IBAction)stopDiscoverAction:(id)sender;
- (IBAction)connectDeviceAction:(id)sender;
- (IBAction)disconnectDeviceAction:(id)sender;
- (IBAction)testLightsAction:(id)sender;
- (IBAction)configWifiAction:(id)sender;
- (IBAction)userDoneEnteringText:(id)sender;

-(AppTableData*)getAppTableData;

@end

