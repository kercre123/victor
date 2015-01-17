//
//  SettingsConnectionViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/15/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "SettingsConnectionViewController.h"
#import "CozmoEngineWrapper.h"
#import "CozmoEngineWrapper+UI.h"
#import "CozmoOperator.h"
#import "NSUserDefaults+UI.h"

// For querying IP address
#import <ifaddrs.h>
#import <arpa/inet.h>

@interface SettingsConnectionViewController () <UITextFieldDelegate>
@property (weak, nonatomic) IBOutlet UILabel* engineStateLabel;


@property (weak, nonatomic) IBOutlet UILabel *hostAddressLabel;
@property (weak, nonatomic) IBOutlet UITextField* hostAddressTextField;

@property (weak, nonatomic) IBOutlet UITextField* vizAddressTextField;
@property (weak, nonatomic) IBOutlet UILabel *vizAddressLabel;

@property (weak, nonatomic) IBOutlet UITextField* engineHeartbeatRateTextField;
@property (weak, nonatomic) IBOutlet UILabel *engineHeartBeatLabel;

@property (weak, nonatomic) IBOutlet UIButton* engineStartStopButton;

@property (strong, nonatomic) NSArray* _orderedTextFields;
@property (weak, nonatomic) UIResponder* _currentUIResponder;

@property (weak, nonatomic) CozmoEngineWrapper* cozmoEngineWrapper;

@property (weak, nonatomic) IBOutlet UISwitch *asHostSwitch;
@property (weak, nonatomic) IBOutlet UILabel *asHostLabel;

@property (weak, nonatomic) IBOutlet UISwitch *cameraResolutionSwitch;
@property (weak, nonatomic) IBOutlet UILabel *cameraResolutionLabel;

@property (weak, nonatomic) IBOutlet UISegmentedControl *numRobotsSelector;
@property (weak, nonatomic) IBOutlet UISegmentedControl *numUiDevicesSelector;

@end

@implementation SettingsConnectionViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  // Do any additional setup after loading the view.

  self.cozmoEngineWrapper = [CozmoEngineWrapper defaultEngine];

  // Setup TextField handling
  self._orderedTextFields = @[ self.hostAddressTextField, self.vizAddressTextField, self.engineHeartbeatRateTextField ];

  self.engineStartStopButton.layer.cornerRadius = 10.0;
  self.engineStartStopButton.layer.borderColor = [UIColor blackColor].CGColor;
  self.engineStartStopButton.layer.borderWidth = 2.0;

//  [self.cameraResolutionSwitch setOn:[NSUserDefaults cameraIsHighResolution]];
  [self.cameraResolutionSwitch setOn:YES];

  // Set Resign First Responder Gesture
  UITapGestureRecognizer *tapGuesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleResignFirstResponderTapGesture:)];
  [self.view addGestureRecognizer:tapGuesture];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];

  // Setup default settings
  [self updateUIObjects];
  [self updateUIWithBasestionState:self.cozmoEngineWrapper.runState];
  // KVO
  [self.cozmoEngineWrapper addObserver:self forKeyPath:@"runState" options:NSKeyValueObservingOptionNew context:nil];
}

- (void)viewWillDisappear:(BOOL)animated
{
  // KVO
  [self.cozmoEngineWrapper removeObserver:self forKeyPath:@"runState" context:nil];

  [super viewWillDisappear:animated];
}


#pragma mark - KVO Methods

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
  if ([object isEqual:self.cozmoEngineWrapper] && [keyPath isEqualToString:@"runState"]) {

    if ([keyPath isEqualToString:@"runState"]) {
      [self updateUIWithBasestionState:self.cozmoEngineWrapper.runState];
    }
  }
}


- (NSString *)getIPAddress
{
  
  NSString* deviceString = (TARGET_IPHONE_SIMULATOR ? @"en1" : @"en0");
  
  NSString *address = @"error";
  struct ifaddrs *interfaces = NULL;
  struct ifaddrs *temp_addr = NULL;
  int success = 0;
  // retrieve the current interfaces - returns 0 on success
  success = getifaddrs(&interfaces);
  if (success == 0) {
    // Loop through linked list of interfaces
    temp_addr = interfaces;
    while(temp_addr != NULL) {
      if(temp_addr->ifa_addr->sa_family == AF_INET) {
        // Check if interface is en0 which is the wifi connection on the iPhone
        // TODO: what about on simulator??
        if([[NSString stringWithUTF8String:temp_addr->ifa_name] isEqualToString:deviceString]) {
          // Get NSString from C String
          address = [NSString stringWithUTF8String:inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr)];
          
        }
        
      }
      
      temp_addr = temp_addr->ifa_next;
    }
  }
  // Free memory
  freeifaddrs(interfaces);
  return address;
  
}

#pragma mark - Config Basestation

- (void)updateBasestationConfig
{
  NSString* ipAddress = self.hostAddressTextField.text;
  [NSUserDefaults setLastHostAdvertisingIP:ipAddress];
  [self.cozmoEngineWrapper setHostAdvertisingIP:ipAddress];
  
  ipAddress = self.vizAddressTextField.text;
  [self.cozmoEngineWrapper setVizIP:ipAddress];
  [NSUserDefaults setLastVizIP:ipAddress];
  
  float heartbeatRate = [self.engineHeartbeatRateTextField.text floatValue];
  [self.cozmoEngineWrapper setHeartbeatRate:(NSTimeInterval)heartbeatRate];
  
  int N = (int)self.numRobotsSelector.selectedSegmentIndex;
  [self.cozmoEngineWrapper setNumRobotsToWaitFor:N];
  [NSUserDefaults setLastNumRobots:N];
  
  N = (int)self.numUiDevicesSelector.selectedSegmentIndex;
  [self.cozmoEngineWrapper setNumUiDevicesToWaitFor:N];
  [NSUserDefaults setLastNumUiDevices:N];
}


#pragma mark - UI Helper Methods

- (void)updateUIObjects
{
  self.engineStateLabel.text = [self basestatoinStateStringWithState:self.cozmoEngineWrapper.runState];
  self.hostAddressTextField.text = self.cozmoEngineWrapper.hostAdvertisingIP;
  self.vizAddressTextField.text  = self.cozmoEngineWrapper.vizIP;
  self.engineHeartbeatRateTextField.text = [self engineHeartbeatRateStringWithValue:self.cozmoEngineWrapper.heartbeatRate];
  [self updateCameraResolutionLabel];
  self.numRobotsSelector.selectedSegmentIndex = self.cozmoEngineWrapper.numRobotsToWaitFor;
  self.numUiDevicesSelector.selectedSegmentIndex = self.cozmoEngineWrapper.numUiDevicesToWaitFor;
}

- (void)updateUIWithBasestionState:(CozmoEngineRunState)state
{
  self.engineStateLabel.text = [self basestatoinStateStringWithState:state];
  [self updateBasestationStartStopButtonWithState:state];

  // Only allow config when state is None
  [self setAllowConfig:(CozmoEngineRunStateNone == state)];
}

- (void)updateCameraResolutionLabel
{
  //self.cameraResolutionLabel.text = self.cameraResolutionSwitch.isOn ? @"High Resolution" : @"Low Resolution";
  self.cameraResolutionLabel.text = self.cameraResolutionSwitch.isOn ? @"Robot Vision ON" : @"Robot Vision OFF";
}

- (void)updateBasestationStartStopButtonWithState:(CozmoEngineRunState)state
{
  NSString *title;
  UIColor *color;
  switch (state) {
    case CozmoEngineRunStateNone:
    {
      title = @"Start Cozmo Engine";
      color = [UIColor greenColor];
    }
      break;

    case CozmoEngineRunStateCommsReady:
    {
      title = @"Stop Cozmo Engine";
      color = [UIColor yellowColor];
    }
      break;

    case CozmoEngineRunStateRobotConnected:
    {
      title = @"Start Cozmo Engine";
      color = [UIColor orangeColor];
    }
      break;

    case CozmoEngineRunStateRunning:
    {
      title = @"Stop Cozmo Engine";
      color = [UIColor redColor];
    }
      break;

    default:
      break;
  }

  [self.engineStartStopButton setTitle:title forState:UIControlStateNormal];
  self.engineStartStopButton.backgroundColor = color;
}

- (NSString*)basestatoinStateStringWithState:(CozmoEngineRunState)state
{
  return [NSString stringWithFormat:@"CozmoEngine State: %@", [self.cozmoEngineWrapper engineStateString]];
}

- (NSString*)engineHeartbeatRateStringWithValue:(double)value
{
  return [NSString stringWithFormat:@"%.2f Hz", value];
}

- (NSString*)robotConnectedStringWithState:(BOOL)connected
{
  return [NSString stringWithFormat:@"Robot Connected: %@", connected ? @"YES" : @"NO"];
}

- (void)setAllowConfig:(BOOL)allow
{
  self.asHostSwitch.enabled = allow;
  self.asHostLabel.enabled = allow;
  
  self.hostAddressTextField.enabled = !self.asHostSwitch.isOn;
  self.hostAddressLabel.enabled = !self.asHostSwitch.isOn;
  
  self.vizAddressTextField.enabled = allow;
  self.vizAddressLabel.enabled = allow;
  
  self.engineHeartbeatRateTextField.enabled = allow;
  self.engineHeartBeatLabel.enabled = allow;
  
  // Note that resolution switch to operate only while engine is running
  // (i.e. reverse of the others) because it actually sends a message to the
  // robot (though it will only have an effect if a robot has connected of course)
  self.cameraResolutionSwitch.enabled = !allow;
  self.cameraResolutionLabel.enabled = !allow;
  
  self.numRobotsSelector.enabled = allow;
  self.numUiDevicesSelector.enabled = allow;
}


#pragma mark - UITextField Delegate Methods

- (void)textFieldDidBeginEditing:(UITextField *)textField
{
  self._currentUIResponder = textField;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  // Handle label special cases
  if ([textField isEqual:self.engineHeartbeatRateTextField]) {
    double heartbeatRate = [self.engineHeartbeatRateTextField.text floatValue];
    self.engineHeartbeatRateTextField.text = [self engineHeartbeatRateStringWithValue:heartbeatRate];
  }

  [self._currentUIResponder resignFirstResponder];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  NSUInteger currentIdx = [self._orderedTextFields indexOfObject:textField];
  if (currentIdx != NSNotFound && currentIdx < self._orderedTextFields.count) {
    UIResponder *nextResponder = self._orderedTextFields[currentIdx + 1];
    [nextResponder becomeFirstResponder];
  }

  return YES;
}


#pragma mark - Handle Action Methods

- (void)handleResignFirstResponderTapGesture:(UITapGestureRecognizer*)gesture
{
  [self._currentUIResponder resignFirstResponder];
}

- (IBAction)handleEngineStartStopButtonPress:(id)sender
{
  // Toggle Basestation states

  switch (self.cozmoEngineWrapper.runState) {
    case CozmoEngineRunStateNone:
    {
      BOOL isHost = self.asHostSwitch.isOn;
      
      [self updateBasestationConfig];
      [self.cozmoEngineWrapper start:isHost];
    }
      break;

    case CozmoEngineRunStateRunning:
    {
      [self.cozmoEngineWrapper stop];
    }
      break;

    default:
      NSLog(@"Unknown CozmoBasestation run state.\n");
      break;
  }
}

- (IBAction)handleCameraResolutionSwitch:(UISwitch*)sender
{
//  [NSUserDefaults setCameraIsHighResolution:sender.isOn];
  [self updateCameraResolutionLabel];
  //[self.cozmoEngineWrapper.cozmoOperator sendCameraResolution:sender.isOn];
  [self.cozmoEngineWrapper.cozmoOperator sendEnableRobotImageStreaming:sender.isOn];
}

- (IBAction)handleAsHostSwitch:(UISwitch *)sender {
  if(sender.isOn == NO) {
    self.asHostLabel.text = @"As Client";
    self.hostAddressTextField.enabled = YES;
    self.hostAddressLabel.enabled = YES;
    self.hostAddressTextField.text = self.cozmoEngineWrapper.hostAdvertisingIP;
  } else {
    self.asHostLabel.text = @"As Host";
    self.hostAddressTextField.enabled = NO;
    self.hostAddressLabel.enabled = NO;
    self.hostAddressTextField.text = [self getIPAddress];
  }
}

@end
