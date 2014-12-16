//
//  SettingsConnectionViewController.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/15/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "SettingsConnectionViewController.h"
#import "CozmoBasestation.h"
#import "CozmoBasestation+UI.h"
#import "NSUserDefaults+UI.h"

@interface SettingsConnectionViewController () <UITextFieldDelegate>
@property (weak, nonatomic) IBOutlet UILabel* basestationStateLabel;


@property (weak, nonatomic) IBOutlet UITextField* hostAddressTextField;
@property (weak, nonatomic) IBOutlet UITextField* basestationAddressTextField;
@property (weak, nonatomic) IBOutlet UITextField* basestationHeartbeatRateTextField;

@property (weak, nonatomic) IBOutlet UIButton* basestationStartStopButton;

@property (strong, nonatomic) NSArray* _orderedTextFields;
@property (weak, nonatomic) UIResponder* _currentUIResponder;

@property (weak, nonatomic) CozmoBasestation* _basestation;

@end

@implementation SettingsConnectionViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  // Do any additional setup after loading the view.

  self._basestation = [CozmoBasestation defaultBasestation];

  // Setup TextField handling
  self._orderedTextFields = @[ self.hostAddressTextField, self.basestationAddressTextField, self.basestationHeartbeatRateTextField ];

  // Setup default settings
  [self updateUIObjects];
  [self updateUIWithBasestionState:self._basestation.runState];

  self.basestationStartStopButton.layer.cornerRadius = 10.0;
  self.basestationStartStopButton.layer.borderColor = [UIColor blackColor].CGColor;
  self.basestationStartStopButton.layer.borderWidth = 2.0;

  // Set Resign First Responder Gesture
  UITapGestureRecognizer *tapGuesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleResignFirstResponderTapGesture:)];
  [self.view addGestureRecognizer:tapGuesture];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];

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
  if ([object isEqual:self._basestation] && [keyPath isEqualToString:@"runState"]) {

    if ([keyPath isEqualToString:@"runState"]) {
      [self updateUIWithBasestionState:self._basestation.runState];
    }
  }
}


#pragma mark - Config Basestation

- (void)updateBasestationConfig
{
  NSString* ipAddress = self.hostAddressTextField.text;
  [self._basestation setHostAdvertisingIP:ipAddress];
  [NSUserDefaults setLastHostAdvertisingIP:ipAddress];

  ipAddress = self.basestationAddressTextField.text;
  [self._basestation setBasestationIP:ipAddress];
  [NSUserDefaults setLastBasestationIP:ipAddress];
  
  float heartbeatRate = [self.basestationHeartbeatRateTextField.text floatValue];
  [self._basestation setHeartbeatRate:(NSTimeInterval)heartbeatRate];
}


#pragma mark - UI Helper Methods

- (void)updateUIObjects
{
  self.basestationStateLabel.text = [self basestatoinStateStringWithState:self._basestation.runState];
  self.hostAddressTextField.text = self._basestation.hostAdvertisingIP;
  self.basestationAddressTextField.text = self._basestation.basestationIP;
  self.basestationHeartbeatRateTextField.text = [self basestationHeartbeatRateStringWithValue:self._basestation.heartbeatRate];
}

- (void)updateUIWithBasestionState:(CozmoBasestationRunState)state
{
  self.basestationStateLabel.text = [self basestatoinStateStringWithState:state];
  [self updateBasestationStartStopButtonWithState:state];

  // Only allow config when state is None
  [self setAllowConfig:(CozmoBasestationRunStateNone == state)];
}

- (void)updateBasestationStartStopButtonWithState:(CozmoBasestationRunState)state
{
  NSString *title;
  UIColor *color;

  switch (state) {
    case CozmoBasestationRunStateNone:
    {
      title = @"Start Comms";
      color = [UIColor greenColor];
    }
      break;

    case CozmoBasestationRunStateCommsReady:
    {
      title = @"Stop Comms";
      color = [UIColor yellowColor];
    }
      break;

    case CozmoBasestationRunStateRobotConnected:
    {
      title = @"Start Basestation";
      color = [UIColor orangeColor];
    }
      break;

    case CozmoBasestationRunStateRunning:
    {
      title = @"Stop Basestation";
      color = [UIColor redColor];
    }
      break;

    default:
      break;
  }

  [self.basestationStartStopButton setTitle:title forState:UIControlStateNormal];
  self.basestationStartStopButton.backgroundColor = color;
}

- (NSString*)basestatoinStateStringWithState:(CozmoBasestationRunState)state
{
  return [NSString stringWithFormat:@"Basestation State: %@", [self._basestation basestationStateString]];
}

- (NSString*)basestationHeartbeatRateStringWithValue:(double)value
{
  return [NSString stringWithFormat:@"%.2f Hz", value];
}

- (NSString*)robotConnectedStringWithState:(BOOL)connected
{
  return [NSString stringWithFormat:@"Robot Connected: %@", connected ? @"YES" : @"NO"];
}

- (void)setAllowConfig:(BOOL)allow
{
  self.hostAddressTextField.enabled = allow;
  self.basestationAddressTextField.enabled = allow;
  self.basestationHeartbeatRateTextField.enabled = allow;
}


#pragma mark - UITextField Delegate Methods

- (void)textFieldDidBeginEditing:(UITextField *)textField
{
  self._currentUIResponder = textField;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  // Handle label special cases
  if ([textField isEqual:self.basestationHeartbeatRateTextField]) {
    double heartbeatRate = [self.basestationHeartbeatRateTextField.text floatValue];
    self.basestationHeartbeatRateTextField.text = [self basestationHeartbeatRateStringWithValue:heartbeatRate];
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

- (IBAction)handleAutoConnectSwitchToggle:(UISwitch*)sender
{
  self._basestation.autoConnect = sender.isOn;
  [NSUserDefaults setAutoConnectRobot:sender.isOn];
}

- (IBAction)handleBasestationStartStopButtonPress:(id)sender
{
  // Toggle Basestation states

  switch (self._basestation.runState) {
    case CozmoBasestationRunStateNone:
    {
      [self updateBasestationConfig];
      [self._basestation startComms];
    }
      break;

    case CozmoBasestationRunStateCommsReady:
    {
      [self._basestation stopCommsAndBasestation];
    }
      break;

    case CozmoBasestationRunStateRobotConnected:
    {
      [self._basestation startBasestation];
    }
      break;

    case CozmoBasestationRunStateRunning:
    {
      [self._basestation stopCommsAndBasestation];
    }
      break;

    default:
      break;
  }
}



@end
