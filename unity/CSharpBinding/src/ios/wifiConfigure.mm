#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <DAS/DAS.h>
#import "wifiConfigure.h"
#import "RoutingHTTPServer.h"

// HTTP static function forward declarations
static void handleMobileconfigRootRequest(RouteResponse * response, NSString* localAddress);
static void handleMobileconfigLoadRequestWithData(RouteResponse * response, NSData * mobileconfigData);
static void handleMobileconfigLoadRequestWithLink(RouteResponse * response);

#pragma mark -------- WifiConfigure class implementation --------

namespace Anki {
namespace Cozmo {
    
struct WifiConfigure::WifiConfigureImpl {
  RoutingHTTPServer * _httpServer = nil;
  NSData * _mobileconfigData = nil;
  NSString* _localAddress = nil;
  bool _shouldInstallProfile = TRUE;
  int _portNum = 0;
};

WifiConfigure::WifiConfigure() : _impl(new WifiConfigureImpl{}) {
  @autoreleasepool {
    _impl->_httpServer = [[RoutingHTTPServer alloc] init];
    
    [_impl->_httpServer handleMethod:@"GET" withPath:@"/start" block:^(RouteRequest *request, RouteResponse *response) {
      _impl->_shouldInstallProfile = TRUE;
      handleMobileconfigRootRequest(response, _impl->_localAddress);
    }];
    
    [_impl->_httpServer handleMethod:@"GET" withPath:@"/load" block:^(RouteRequest *request, RouteResponse *response) {
      if(_impl->_shouldInstallProfile) {
        _impl->_shouldInstallProfile = FALSE;
        handleMobileconfigLoadRequestWithData(response, _impl->_mobileconfigData);
      } else {
        handleMobileconfigLoadRequestWithLink(response);
      }
    }];
    
    NSError *err = nil;
    if (![_impl->_httpServer start:&err]) {
      NSString* errorString = [NSString stringWithFormat:@"%@", err];
      DASError("WifiConfigure.constructor", "Error starting http server: %s", [errorString UTF8String]);
      
      [_impl->_httpServer stop];
      _impl->_httpServer = nil;
    } else {
      // We allow the server to grab whatever available port, then we store it here
      _impl->_portNum = [_impl->_httpServer listeningPort];
      _impl->_localAddress = [@"http://localhost:" stringByAppendingString:[NSString stringWithFormat:@"%d", _impl->_portNum]];
    }
  } // autoreleasepool
}

WifiConfigure::~WifiConfigure() {
  @autoreleasepool {
    if (_impl->_httpServer != nil) {
      [_impl->_httpServer stop];
      // Not deallocing here because we are ARC
    }
    
    _impl->_httpServer = nil;
    _impl->_mobileconfigData = nil;
    _impl->_localAddress = nil;
  } // autoreleasepool
}

bool WifiConfigure::UpdateMobileconfig(const char* wifiSSID, const char* wifiPasskey) {
  @autoreleasepool {
    NSString* configPath = [[NSBundle mainBundle] pathForResource:@"CozmoWifiConfigTemplate" ofType:@"mobileconfig"];
    if (![[NSFileManager defaultManager] fileExistsAtPath:configPath]) {
      DASError("WifiConfigure.UpdateMobileconfig","Could not find wifi config template at path %s", [configPath UTF8String]);
      return FALSE;
    }
    
    _impl->_mobileconfigData = [NSData dataWithContentsOfFile:configPath];
    
    if (nil == _impl->_mobileconfigData) {
      DASError("WifiConfigure.UpdateMobileconfig", "Problem loading mobileconfig template at path: %s", [configPath UTF8String]);
      return FALSE;
    }
    
    NSError* error = nil;
    NSMutableDictionary* configDict = (NSMutableDictionary*) [NSPropertyListSerialization propertyListWithData:_impl->_mobileconfigData
                                                                                                       options:NSPropertyListMutableContainersAndLeaves
                                                                                                        format:nil
                                                                                                         error:&error];
    if (nil == configDict) {
      NSString* errorString = [NSString stringWithFormat:@"%@", error];
      DASError("WifiConfigure.UpdateMobileconfig", "Problem deserializing mobileconfig template: %s", [errorString UTF8String]);
      return FALSE;
    }
    
    static NSString* const ssidKey = @"SSID_STR";
    static NSString* const passwordKey = @"Password";
    static NSString* const payloadKey = @"PayloadContent";
    static NSString* const uuidKey = @"PayloadUUID";
    static const int payloadIndex = 0;
    
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
      DASError("WifiConfigure.UpdateMobileconfig", "Problem creating mobileconfig from modified plist: %s", [errorString UTF8String]);
      return FALSE;
    }
    
    // Finally set us to use the data we've prepared
    _impl->_mobileconfigData = preparedConfig;
    return TRUE;
  } // autoreleasepool
}

void WifiConfigure::InstallMobileconfig() {
  @autoreleasepool {
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString: [_impl->_localAddress stringByAppendingString:@"/start/"]]];
  } // autoreleasepool
}
  
} // namespace Cozmo
} // namespace Anki


#pragma mark -------- HTTP response functionality --------

static void handleMobileconfigRootRequest(RouteResponse * response, NSString* localAddress) {
  @autoreleasepool {
    NSString* myResponse =
    @"<HTML><HEAD><title>Profile Install</title>\
    </HEAD><script> \
    function load() { \
    window.location.href='";
    
    NSString* responseEnd = @"/load/'; \
    } \
    var int=self.setInterval(function(){load()},1000); \
    </script><BODY></BODY></HTML>";
    
    myResponse = [myResponse stringByAppendingString:localAddress];
    myResponse = [myResponse stringByAppendingString:responseEnd];
    
    [response respondWithString: myResponse];
  } // autoreleasepool
}

static void handleMobileconfigLoadRequestWithData(RouteResponse * response, NSData * mobileconfigData) {
  @autoreleasepool {
    [response setHeader:@"Content-Type" value:@"application/x-apple-aspen-config"];
    [response respondWithData:mobileconfigData];
  }
}

static void handleMobileconfigLoadRequestWithLink(RouteResponse * response) {
  @autoreleasepool {
    NSString* myHTTPResponse =
    @"<HTML><HEAD><title>Profile Install</title>\
    </HEAD><script> \
    </script><BODY><h1><a href='cozmoApp://'>Return To Cozmo</a></h1></BODY></HTML>";
    [response respondWithString: myHTTPResponse];
  } // autoreleasepool
}
