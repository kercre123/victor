#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "wifiConfigure.h"
#import "RoutingHTTPServer.h"

static RoutingHTTPServer * _httpServer = nil;
static NSData * _mobileconfigData = nil;
static NSString* _localAddress;
static bool _shouldInstallProfile = TRUE;
static int _portNum = 0;


static void handleMobileconfigRootRequest(RouteRequest *request, RouteResponse * response) {
  NSLog(@"handleMobileconfigRootRequest");
  
  _shouldInstallProfile = TRUE;
  
  NSString* myResponse =
  @"<HTML><HEAD><title>Profile Install</title>\
  </HEAD><script> \
  function load() { \
  window.location.href='";
  
  NSString* responseEnd = @"/load/'; \
  } \
  var int=self.setInterval(function(){load()},1000); \
  </script><BODY></BODY></HTML>";
  
  myResponse = [myResponse stringByAppendingString:_localAddress];
  myResponse = [myResponse stringByAppendingString:responseEnd];
  
  [response respondWithString: myResponse];
}

static void handleMobileconfigLoadRequest(RouteRequest * request, RouteResponse * response) {
  if(_shouldInstallProfile) {
    NSLog(@"handleMobileconfigLoadRequest, first time");
    _shouldInstallProfile = FALSE;
    
    [response setHeader:@"Content-Type" value:@"application/x-apple-aspen-config"];
    [response respondWithData:_mobileconfigData];
  } else {
    NSLog(@"handleMobileconfigLoadRequest, NOT first time");
    NSString* myHTTPResponse =
    @"<HTML><HEAD><title>Profile Install</title>\
    </HEAD><script> \
    </script><BODY><h1><a href='cozmo://'>Return To Cozmo</a></h1></BODY></HTML>";
    [response respondWithString: myHTTPResponse];
  }
}

int COZHttpServerInit(int portNum)
{
  _portNum = portNum;
  _localAddress = [@"http://localhost:" stringByAppendingString:[NSString stringWithFormat:@"%d", _portNum]];
  
  _httpServer = [[RoutingHTTPServer alloc] init];
  [_httpServer setPort:_portNum];              // TODO: make sure this port isn't already in use
  
  [_httpServer handleMethod:@"GET" withPath:@"/start" block:^(RouteRequest *request, RouteResponse *response) {
    handleMobileconfigRootRequest(request, response);
  }];
   
  [_httpServer handleMethod:@"GET" withPath:@"/load" block:^(RouteRequest *request, RouteResponse *response) {
    handleMobileconfigLoadRequest(request, response);
  }];
  
  NSError *err = nil;
  if (![_httpServer start:&err]) {
    NSLog(@"Error starting http server: %@", err);
    
    [_httpServer stop];
    _httpServer = nil;
  }
  
  return 0;
}

int COZHttpServerShutdown() {
  if (_httpServer != nil) {
    [_httpServer stop];
    // Not deallocing here because we are ARC
  }
  
  _httpServer = nil;
  _mobileconfigData = nil;
  _localAddress = nil;
  
  return 0;
}

int COZWifiConfigure(const char* wifiSSID, const char* wifiPasskey) {
  // TODO: Instead of just loading a static file, load it then modify with correct data given ssid passed in.
  NSMutableString* path = [NSMutableString stringWithString:[[NSBundle mainBundle] bundlePath]];
  [path appendString:@"/Cozmos.mobileconfig"];
  _mobileconfigData = [NSData dataWithContentsOfFile:path];
  
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString: [_localAddress stringByAppendingString:@"/start/"]]];
  return 0;
}