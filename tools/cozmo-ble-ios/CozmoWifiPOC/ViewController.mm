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
#import "BLECozmoConnection.h"
#import <UIKit/UIKit.h>

@interface ViewController ()

@property (nonatomic, strong) AppTableData* appTableData;
@property (nonatomic, weak) AppDelegate* appDelegate;
@property (strong, nonatomic) NSMutableArray* cozmoConnections;
@property (nonatomic, weak) BLECozmoConnection* lastSelectedConnection;

@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  _appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
  _appTableData = [[AppTableData alloc] initWithViewController:self];
  _idListTable.dataSource = _appTableData;
  _idListTable.delegate = _appTableData;
  _cozmoConnections = [NSMutableArray array];
  _lastSelectedConnection = nil;
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

- (IBAction)startDiscoverAction:(id)sender {
  [_appDelegate stopDiscovering];
  [self clearList];
  [_appDelegate startDiscovering];
}

- (IBAction)stopDiscoverAction:(id)sender {
  [_appDelegate stopDiscovering];
}

- (IBAction)connectDeviceAction:(id)sender {
  if (_lastSelectedConnection)
  {
    [_appDelegate doDisconnection:_lastSelectedConnection];
    _lastSelectedConnection = nil;
  }
  _lastSelectedConnection = [self getSelectedCozmo];
  if (!_lastSelectedConnection)
  {
    return;
  }
  [_appDelegate doConnection:_lastSelectedConnection];
}

- (IBAction)disconnectDeviceAction:(id)sender {
  if (_lastSelectedConnection)
  {
    [_appDelegate doDisconnection:_lastSelectedConnection];
    [self removeCozmo:_lastSelectedConnection];
    _lastSelectedConnection = nil;
  }
}

- (IBAction)testLightsAction:(id)sender {
  if (_lastSelectedConnection)
  {
    [_appDelegate doSendTestLightsMessage:_lastSelectedConnection];
  }
}

- (IBAction)configWifiAction:(id)sender {
  if (_lastSelectedConnection)
  {
    NSString* ssid = _ssidTextField.text;
    NSString* password = _passwordTextField.text;
    if (ssid && password)
    {
      [_appDelegate doConfigWifiMessage:_lastSelectedConnection ssid:ssid password:password];
    }
  }
}

-(IBAction)userDoneEnteringText:(id)sender
{
}

-(void)addCozmo:(BLECozmoConnection*)connection
{
  dispatch_async(dispatch_get_main_queue(), ^{
    if (![_cozmoConnections containsObject:connection])
    {
      [_cozmoConnections addObject:connection];
      [_idListTable reloadData];
    }
  });
}

-(void)removeCozmo:(BLECozmoConnection*)connection
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [_cozmoConnections removeObject:connection];
    [_idListTable reloadData];
  });
}

-(void)clearList
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [_cozmoConnections removeAllObjects];
    [_idListTable reloadData];
  });
}

-(BLECozmoConnection*)getSelectedCozmo
{
  NSIndexPath* path = [_idListTable indexPathForSelectedRow];
  if (!path) return nil;
  
  return [_cozmoConnections objectAtIndex:path.row];
}

-(NSUInteger)getNumRows
{
  return [_cozmoConnections count];
}

-(NSString*) getStringForRow:(NSUInteger)row
{
  NSUUID* uuid = ((BLECozmoConnection*)[_cozmoConnections objectAtIndex:row]).carPeripheral.identifier;
  //NSString* uuidString = [uuid UUIDString];
  //return [NSString stringWithFormat:@"0x%llx-%@", ((BLECozmoConnection*)[_cozmoConnections objectAtIndex:row]).mfgID, uuidString];
  uint8_t uuidBytes[16]{};
  [uuid getUUIDBytes:uuidBytes];
  // Represent each connection as the first 4 bytes of the peripheral UUID in hex
  NSString* shortUuidString = [NSString stringWithFormat:@"%02X%02X%02X%02X", uuidBytes[0], uuidBytes[1], uuidBytes[2], uuidBytes[3]];
  return shortUuidString;
}
@end
