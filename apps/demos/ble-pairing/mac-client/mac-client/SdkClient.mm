//
//  SdkClient.m
//  mac-client
//
//  Created by Paul Aluri on 9/21/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <SecurityInterface/SFCertificateTrustPanel.h>
#import <string>
#import "SdkClient.h"

@implementation SdkClient

-(id) initWithEsn:(NSString*)esn ipAddr:(NSString*)ipAddr clientAppToken:(NSString*)clientAppToken {
  self = [super init];
  
  if(self) {
    _esn = esn;
    _ip = ipAddr;
    _clientAppToken = clientAppToken;
    _responseSemaphore = dispatch_semaphore_create(0);
    
    NSURLSessionConfiguration* sessionConfig = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSURLSession* session = [NSURLSession sessionWithConfiguration:sessionConfig delegate:self delegateQueue:nil];
    
    NSURLRequest* request = [NSURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithFormat:@"https://%@/v1/protocol_version", _ip]]];
    NSMutableURLRequest* mRequest = [request mutableCopy];
    [mRequest setHTTPMethod:@"POST"];
    NSString* emptyJson = @"{}";
    [mRequest setHTTPBody:[emptyJson dataUsingEncoding:NSUTF8StringEncoding]];
    [mRequest addValue:[NSString stringWithFormat:@"Bearer %@", _clientAppToken] forHTTPHeaderField:@"Authorization"];
    
    int code = 0;
    int* statusCodePtr = &code;
    
    [[session dataTaskWithRequest:mRequest completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
      NSHTTPURLResponse* res = (NSHTTPURLResponse*)response;
      dispatch_semaphore_signal(_responseSemaphore);
      *statusCodePtr = (int)res.statusCode;
    }] resume];
    
    if(dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]) == 0) {
      // got response
      printf("  => WiFi SDK comms connectability [success=%s status=\033[0;%d;%dm%d\033[0m]\n", code==200?"OK":"failed", 49, code==200?92:91, code);
    } else {
      // timed out
      printf("  => WiFi SDK comms test timed out.\n");
    }
  }
  
  return self;
}

-(dispatch_time_t) getTimeout {
  return dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5 * NSEC_PER_SEC));
}

-(NSData*)getDerDataFromPem:(NSString*)contents {
  NSString* localCertS = contents;
  std::string localCertStr = std::string([localCertS UTF8String]);
  std::string head = "-----BEGIN CERTIFICATE-----\n";
  std::string tail = "-----END CERTIFICATE-----";

  std::string::size_type idxHead = localCertStr.find(head, 0);
  std::string::size_type idxTail = localCertStr.find(tail, 0);
  
  if(idxHead == std::string::npos ||
     idxTail == std::string::npos) {
    return nil;
  }
  
  // substring
  localCertStr = localCertStr.substr(idxHead + head.length(), localCertStr.length() - (idxHead + head.length()) - (localCertStr.length() - idxTail));
  
  NSString* localCertNS = [NSString stringWithUTF8String:localCertStr.c_str()];
  
  // decode base 64
  return [[NSData alloc] initWithBase64EncodedString:localCertNS options:NSDataBase64DecodingIgnoreUnknownCharacters];
}

- (void)URLSession:(NSURLSession *)session
didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
 completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential))completionHandler {
  // download cert
  NSURL* certUrl = [NSURL URLWithString:[NSString stringWithFormat:@"%@%@", @"https://session-certs.token.global.anki-services.com/vic/", _esn]];
  NSString* certData = [NSString stringWithContentsOfURL:certUrl encoding:NSUTF8StringEncoding error:nil];
  NSData* data = [self getDerDataFromPem:certData];

  
  if(challenge.protectionSpace.authenticationMethod != NSURLAuthenticationMethodServerTrust) {
    completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, NULL);
    return;
  }
  
  SecTrustRef serverTrust = challenge.protectionSpace.serverTrust;
  SecTrustResultType result;
  SecTrustEvaluate(serverTrust, &result);
  SecCertificateRef cert = SecTrustGetCertificateAtIndex(serverTrust, 0);
  
  // Get local and remote cert data
  NSData *remoteCertificateData = CFBridgingRelease(SecCertificateCopyData(cert));
  NSData *localCertificate = data;
  
  // The pinnning check
  if ([remoteCertificateData isEqualToData:localCertificate]) {
    NSURLCredential *credential = [NSURLCredential credentialForTrust:serverTrust];
    completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
  } else {
    completionHandler(NSURLSessionAuthChallengeCancelAuthenticationChallenge, NULL);
  }
}

@end
