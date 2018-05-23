//
//  ViewController.m
//  Han
//
//  Created by Paul Aluri on 5/23/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import "ViewController.h"
#import "BleCentral.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  // Do any additional setup after loading the view, typically from a nib.
  BleCentral* central = [[BleCentral alloc] init];
  
  NSString* name = [NSString stringWithUTF8String:"W7T8"];
  [central StartScanning:name];
}


- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}


@end
