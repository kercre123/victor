#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <DAS/DAS.h>
#import "wifiConfigure.h"
#import "RoutingHTTPServer.h"

static RoutingHTTPServer * _httpServer = nil;
static NSData * _mobileconfigData = nil;
static NSString* _localAddress;
static bool _shouldInstallProfile = TRUE;
static int _portNum = 0;


static void handleMobileconfigRootRequest(RouteRequest *request, RouteResponse * response) {
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
    _shouldInstallProfile = FALSE;
    
    [response setHeader:@"Content-Type" value:@"application/x-apple-aspen-config"];
    [response respondWithData:_mobileconfigData];
  } else {
    NSString* myHTTPResponse =
    @"<HTML><HEAD><title>Profile Install</title>\
    </HEAD><script> \
    </script><BODY><h1><a href='cozmoApp://'>Return To Cozmo</a></h1></BODY></HTML>";
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
    NSString* errorString = [NSString stringWithFormat:@"%@", err];
    DASError("cozmo.ios.httpServerInit", "Error starting http server: %s", [errorString UTF8String]);
    
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

bool COZWifiConfigure(const char* wifiSSID, const char* wifiPasskey) {
  NSString* configPath = [[NSBundle mainBundle] pathForResource:@"CozmoWifiConfigTemplate" ofType:@"mobileconfig"];
  if (![[NSFileManager defaultManager] fileExistsAtPath:configPath]) {
    DASError("cozmo.ios.wifiConfigure","Could not find wifi config template at path %s", [configPath UTF8String]);
    return FALSE;
  }
  
  _mobileconfigData = [NSData dataWithContentsOfFile:configPath];
  
  NSError* error = nil;
  NSMutableDictionary* configDict = (NSMutableDictionary*) [NSPropertyListSerialization propertyListWithData:_mobileconfigData
                                                                                                     options:NSPropertyListMutableContainersAndLeaves
                                                                                                      format:nil
                                                                                                       error:&error];
  if (nil == configDict) {
    NSString* errorString = [NSString stringWithFormat:@"%@", error];
    DASError("cozmo.ios.wifiConfigure", "Problem deserializing mobileconfig template: %s", [errorString UTF8String]);
    return FALSE;
  }

  NSString* ssidKey = @"SSID_STR";
  NSString* passwordKey = @"Password";
  NSString* payloadKey = @"PayloadContent";
  NSString* uuidKey = @"PayloadUUID";
  int payloadIndex = 0;
  
  // First update the UUID of the mobileconfig to match the unique app install uuid
  NSString* deviceIDString = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
  [configDict setValue:deviceIDString forKey:uuidKey];
  
  // Grab the dict with the connection-specific data
  NSMutableDictionary* cozmoConfig = (NSMutableDictionary*)[(NSMutableArray*)[configDict objectForKey:payloadKey] objectAtIndex: payloadIndex];
  
  // Update the values we care about for the connection
  [cozmoConfig setValue:deviceIDString forKey:uuidKey];
  [cozmoConfig setValue:[NSString stringWithUTF8String:wifiSSID] forKey:ssidKey];
  [cozmoConfig setValue:[NSString stringWithUTF8String:wifiPasskey] forKey:passwordKey];
  
  // Turn our newly updated plist back into some raw data we'll serve
  NSData* preparedConfig = [NSPropertyListSerialization dataWithPropertyList:configDict
                                                                      format:NSPropertyListXMLFormat_v1_0
                                                                     options:(NSPropertyListWriteOptions)0
                                                                       error:&error];
  if (nil == preparedConfig) {
    NSString* errorString = [NSString stringWithFormat:@"%@", error];
    DASError("cozmo.ios.wifiConfigure", "Problem creating mobileconfig from modified plist: %s", [errorString UTF8String]);
    return FALSE;
  }
  
  // Finally set us to use the data we've prepared
  _mobileconfigData = preparedConfig;
  
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString: [_localAddress stringByAppendingString:@"/start/"]]];
  return TRUE;
}

