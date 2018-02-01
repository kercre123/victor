//
//  ViewController.m
//  mDNSServer
//
//  Created by Paul Aluri on 1/29/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import "ViewController.h"
#import <dns_sd.h>

@implementation ViewController

void HandleReply(DNSServiceRef ref, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char* name, const char* regtype, const char* domain, void* context) {
  NSLog(@"Got a call");
}

- (void)viewDidLoad {
  [super viewDidLoad];

  // Do any additional setup after loading the view.
  /*DNSServiceRef ref;
  DNSServiceRegisterReply reply = &HandleReply;
  DNSServiceErrorType res = DNSServiceRegister(&ref, 0, 0, NULL, "_victor._tcp", NULL, NULL, 5353, 0, NULL, reply, NULL);*/
  
  NSNetService* service = [[NSNetService alloc] initWithDomain:@"" type:@"_victor._tcp" name:@"Victor OS" port:0];
  [service setDelegate:self];
  [service publish];
  
  //NSLog(@"response: %d", res);
}


- (void)setRepresentedObject:(id)representedObject {
  [super setRepresentedObject:representedObject];

  // Update the view, if already loaded.
}

- (void)netService:(NSNetService *)sender
didAcceptConnectionWithInputStream:(NSInputStream *)inputStream
      outputStream:(NSOutputStream *)outputStream {
  NSLog(@"connected!");
}


@end
