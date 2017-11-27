/**
 * File: dasPostToServer.mm
 *
 * Author: seichert
 * Created: 01/21/2015
 *
 * Description: Function to be implemented on a per platform basis
 *              to do an HTTP POST to a given URL with the supplied
 *              postBody
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#import <Foundation/Foundation.h>
#include "dasPostToServer.h"
#include <string>
#include <stdio.h>
#import "NSData+Base64.h"
#import "NSString+URLEncoding.h"


// Stubbed out delegate to handle looking at the xml formatted response from aws
@interface DasResponseDelegate : NSObject<NSXMLParserDelegate>
@property bool errorFound;
- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI
        qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict;
@end


@implementation DasResponseDelegate

- (instancetype)init {
  self = [super init];
  if (self) {
    _errorFound = false;
  }
  return self;
}

- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI
        qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict {
  if ( [elementName isEqualToString:@"ErrorResponse"]) {
    _errorFound = true;
    return;
  }
}

@end


#ifdef __cplusplus
extern "C" {
#endif

bool dasPostToServer(const std::string& url, const std::string& postBody, std::string& out_response)
{
  // Needs an autoreleasepool because the NSData_Base64 base64EncodedStringWithData method does a NSString alloc.
  @autoreleasepool {
    NSString* nsUrlString = [NSString stringWithCString:url.c_str() encoding:NSUTF8StringEncoding];
    NSURL* nsUrl = [[NSURL alloc] initWithString: nsUrlString];
    NSData* nsPostBody = [[NSString stringWithCString:postBody.c_str() encoding:NSUTF8StringEncoding] dataUsingEncoding:NSUTF8StringEncoding];
    NSMutableURLRequest* request = [[NSMutableURLRequest alloc] initWithURL:nsUrl];
  
    NSString* payloadBase64 = [NSData_Base64 base64EncodedStringWithData:nsPostBody];
    NSString* payloadStr = [NSString_URLEncoding urlEncodeString: payloadBase64
                                                   usingEncoding:NSUTF8StringEncoding];
    NSString* postStr = [NSString stringWithFormat:@"Action=SendMessage&Version=2011-10-01&MessageBody=%@", payloadStr];
    NSData* finalPostBody = [postStr dataUsingEncoding:NSUTF8StringEncoding];
    request.HTTPBody = finalPostBody;
    request.HTTPMethod = @"POST";
    [request setValue:[NSString stringWithFormat:@"%lu", (unsigned long)finalPostBody.length] forHTTPHeaderField:@"Content-Length"];
    [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];

    __block NSError* error = nil;
    __block NSURLResponse* response = nil;
    __block NSData* responseData = nil;

    dispatch_semaphore_t completionSignal = dispatch_semaphore_create(0);
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
      NSURLSessionTask* task = [[NSURLSession sharedSession] dataTaskWithRequest:request
                                  completionHandler:^(NSData *data, NSURLResponse *blockResponse, NSError *blockError) {
                                    error = blockError;
                                    response = blockResponse;
                                    responseData = [[NSData alloc] initWithData:data];
                                    dispatch_semaphore_signal(completionSignal);
      }];
      [task resume];
    });
    dispatch_semaphore_wait(completionSignal, dispatch_time(DISPATCH_TIME_NOW, 60.0 * NSEC_PER_SEC));
    if(!responseData || error) {
        // TODO: set a reachability handler to upload the events ASAP.
        NSLog(@"Error! Unable to upload DAS events: %@", error);
        return false;
    }
    
    out_response = std::string([[[NSString alloc] initWithData:responseData encoding:NSUTF8StringEncoding] UTF8String]);
    DasResponseDelegate* responseDelegate = [[DasResponseDelegate alloc] init];
    NSXMLParser* responseParser = [[NSXMLParser alloc] initWithData:responseData];
    [responseParser setDelegate:responseDelegate];
    bool success = [responseParser parse];
    if (!success || responseDelegate.errorFound)
    {
      NSLog(@"Error! Unable to upload DAS events: %@", [[NSString alloc] initWithData:responseData encoding:NSUTF8StringEncoding]);
      return false;
    }
    
    // TODO: what happens when we get a non-2xx response?
    return true;
  } // @autoreleasepool
}

#ifdef __cplusplus
}
#endif

