//
//  main.m
//  mDNSService
//
//  Created by Paul Aluri on 1/29/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <dns_sd.h>

void HandleReply(DNSServiceRef ref, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char* name, const char* regtype, const char* domain, void* context) {
  NSLog(@"Got a call");
}

int main(int argc, const char * argv[]) {
  @autoreleasepool {
    // insert code here...
    NSLog(@"Hello, World!");
    
    DNSServiceRef* ref = NULL;
    DNSServiceRegisterReply reply = &HandleReply;
    DNSServiceRegister(ref, 0, 0, "victor", "_http._tcp", NULL, NULL, 5353, 0, NULL, reply, NULL);
  }
  return 0;
}
