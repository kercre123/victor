#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <DAS/DAS.h>
#import "RoutingHTTPServer.h"


#import "HTTPConnection.h"
#import "HTTPDataResponse.h"
#import "HTTPMessage.h"

// No really include things so you don't crash
#import "DDNumber.h"


@interface MyHTTPConnection : HTTPConnection

@end


@implementation MyHTTPConnection

- (BOOL)supportsMethod:(NSString *)method atPath:(NSString *)path
{
  printf("supportsMethod\n");
  // Add support for POST
  
  if ([method isEqualToString:@"POST"])
  {
    // Let's be extra cautious, and make sure the upload isn't 5 gigs
    return requestContentLength < 50;
  }
  if ([method isEqualToString:@"GET"])
  {
    // Let's be extra cautious, and make sure the upload isn't 5 gigs
    return YES;
  }
  return [super supportsMethod:method atPath:path];
}

- (BOOL)expectsRequestBodyFromMethod:(NSString *)method atPath:(NSString *)path
{
  printf("expectsRequestBodyFromMethod\n");
  // Inform HTTP server that we expect a body to accompany a POST request
  if([method isEqualToString:@"POST"])
    return YES;
  
  return [super expectsRequestBodyFromMethod:method atPath:path];
}

- (NSObject<HTTPResponse> *)httpResponseForMethod:(NSString *)method URI:(NSString *)path
{
  printf("httpResponseForMethod\n");
  if ([method isEqualToString:@"POST"] )
  {

    NSString *postStr = nil;
    
    NSData *postData = [request body];
    if (postData)
    {
      postStr = [[NSString alloc] initWithData:postData encoding:NSUTF8StringEncoding];
    }
    
    // Result will be of the form "answer=..."
    
    int answer = [[postStr substringFromIndex:7] intValue];
    
    NSData *response = nil;
    if(answer == 10)
    {
      response = [@"<html><body>Correct<body></html>" dataUsingEncoding:NSUTF8StringEncoding];
    }
    else
    {
      response = [@"<html><body>Sorry - Try Again<body></html>" dataUsingEncoding:NSUTF8StringEncoding];
    }
    
    return [[HTTPDataResponse alloc] initWithData:response];
  }
  else if([method isEqualToString:@"GET"] )
  {
    NSData *get_response = [@"<html><body>GET RESPONSE<body></html>" dataUsingEncoding:NSUTF8StringEncoding];
    return [[HTTPDataResponse alloc] initWithData:get_response];
  }
  
  return [super httpResponseForMethod:method URI:path];
}

@end

HTTPServer* s_HttpServer = nil;
HTTPConnection* s_HttpConnection = nil;

/*static void handleUSBTunnelGetTest(RouteResponse * response, NSString* localAddress) {
  @autoreleasepool {
    [response respondWithString: @"Hello World! Please work"];
  } // autoreleasepool
}*/

void CreateUSBTunnelServerImpl()
{
  printf("CreateUSBTunnelServerImpl\n");
  //@autoreleasepool {
    s_HttpServer = [[HTTPServer alloc] init];
    [s_HttpServer setType:@"_http._tcp."];
    /*[s_HttpServer handleMethod:@"GET" withPath:@"/start" block:^(RouteRequest *request, RouteResponse *response) {
      handleUSBTunnelGetTest(response, "localhost:2223");
    }];*/
    
    /*[_impl->_httpServer handleMethod:@"GET" withPath:@"/load" block:^(RouteRequest *request, RouteResponse *response) {
      if(_impl->_shouldInstallProfile) {
        _impl->_shouldInstallProfile = FALSE;
        handleMobileconfigLoadRequestWithData(response, _impl->_mobileconfigData);
      } else {
        handleMobileconfigLoadRequestWithLink(response);
      }
    }];*/
  
  // Force Import this method to try to avoid crashing...
  // HttpConnection.m copied.
  UInt64 r1;
  NSString* contentLength = @"6";
  BOOL hasR1 = [NSNumber parseString:(NSString *)contentLength intoUInt64:&r1];
  if( hasR1)
  {
    printf("yay\n");
  }
  else
  {
    printf("nay\n");
  }
  
    NSError *err = nil;
    [s_HttpServer setPort:2223];
    
    [s_HttpServer setConnectionClass:[MyHTTPConnection class]];
    
    if (![s_HttpServer start:&err]) {
      NSString* errorString = [NSString stringWithFormat:@"%@", err];
      DASError("WifiConfigure.constructor", "Error starting http server: %s", [errorString UTF8String]);
      
      [s_HttpServer stop];
      s_HttpServer = nil;
    } else {
      printf("Yay Created\n");
      // We allow the server to grab whatever available port, then we store it here
      //_impl->_portNum = [_impl->_httpServer listeningPort];
      //_impl->_localAddress = [@"http://localhost:" stringByAppendingString:[NSString stringWithFormat:@"%d", _impl->_portNum]];
    }
  //} // autoreleasepool
}
void CreateUSBTunnelServer()
{
  CreateUSBTunnelServerImpl();
}
