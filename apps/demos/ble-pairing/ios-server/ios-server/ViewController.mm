//
//  ViewController.m
//  ios-server
//
//  Created by Paul Aluri on 1/9/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import "ViewController.h"
#include <string>
#include <iostream>
#include <NetworkExtension/NetworkExtension.h>
#include "networking/Switchboard.h"

@interface ViewController () {
  
}

@property (weak, nonatomic) IBOutlet UILabel *pinLabel;
@property (weak, nonatomic) IBOutlet UILabel *sharedSecretLabel;

@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  
  Log::Write("ViewController");
  
  pairingQueue = dispatch_queue_create("pairingQueue", NULL);
  Anki::Switchboard::SetQueue(pairingQueue);
  
  dispatch_async(pairingQueue, ^(){
    Log::Write("PairingQueue");
    
    
    Anki::Switchboard::OnPinUpdatedEvent().SubscribeForever(^(std::string pin) {
      dispatch_async(dispatch_get_main_queue(), ^() {
        _pinLabel.text = [NSString stringWithUTF8String:pin.c_str()];
      });
    });
    Anki::Switchboard::Start();
  });
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

@end
