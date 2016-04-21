//
//  ViewController.m
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 1/11/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import "ViewController.h"
#import "AppDelegate.h"
#import "AppTableData.h"
#import <UIKit/UIKit.h>

@interface ViewController ()

@property (nonatomic, strong) AppTableData* appTableData;
@property (nonatomic, weak) AppDelegate* appDelegate;
@property (assign, nonatomic) UInt64 lastConnectMfgID;

@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  _appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
  _appTableData = [[AppTableData alloc] initWithTableView:_idListTable];
  _idListTable.dataSource = _appTableData;
  _idListTable.delegate = _appTableData;
  _lastConnectMfgID = 0;
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

- (IBAction)startDiscoverAction:(id)sender {
  [_appDelegate stopDiscovering];
  [_appTableData clearList];
  [_appDelegate startDiscovering];
}

- (IBAction)stopDiscoverAction:(id)sender {
  [_appDelegate stopDiscovering];
}

- (IBAction)connectDeviceAction:(id)sender {
  if (0 != _lastConnectMfgID)
  {
    [_appDelegate doDisconnection:_lastConnectMfgID];
    _lastConnectMfgID = 0;
  }
  NSString* selectedName = [_appTableData getSelectedCozmo];
  if (nil == selectedName)
  {
    return;
  }
  _lastConnectMfgID = strtoull([selectedName UTF8String], NULL, 16);
  [_appDelegate doConnection:_lastConnectMfgID];
}

- (IBAction)disconnectDeviceAction:(id)sender {
  if (0 != _lastConnectMfgID)
  {
    [_appDelegate doDisconnection:_lastConnectMfgID];
    [_appTableData removeCozmo:[NSString stringWithFormat:@"0x%llx", _lastConnectMfgID]];
    _lastConnectMfgID = 0;
  }
}

- (IBAction)testLightsAction:(id)sender {
  if (0 != _lastConnectMfgID)
  {
    [_appDelegate doSendTestLightsMessage:_lastConnectMfgID];
  }
}

- (IBAction)configWifiAction:(id)sender {
  if (0 != _lastConnectMfgID)
  {
    NSString* ssid = _ssidTextField.text;
    NSString* password = _passwordTextField.text;
    if (ssid && password)
    {
      [_appDelegate doConfigWifiMessage:_lastConnectMfgID ssid:ssid password:password];
    }
  }
}

-(IBAction)userDoneEnteringText:(id)sender
{
}

-(AppTableData*)getAppTableData {
  return _appTableData;
}
@end
