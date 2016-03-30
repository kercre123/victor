#if TARGET_OS_IPHONE
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "RoutingHTTPServer.h"


#import "HTTPConnection.h"
#import "HTTPDataResponse.h"
#import "HTTPMessage.h"

#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/fileUtils/fileUtils.h"


// TODO: move to class.
Anki::Util::Data::DataPlatform* s_dataPlatform;
Anki::Cozmo::IExternalInterface* s_externalInterface;

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
    return requestContentLength < 500000000;
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
      // const std::string animationFolder = _context->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources, animationDir);
      //std::string full_path = s_dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, "assets/animations/");
      // TODO: get the name from json... if parsed to json can just use data platform write to json
      //full_path += "some_anim_name.json";
      std::string full_path = s_dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, "some_anim_name.json");
      printf("Write To: %s\n",full_path.c_str());
      std::string content = [postStr UTF8String];
      Anki::Util::FileUtils::WriteFile(full_path, content);
      
      
      Anki::Cozmo::ExternalInterface::ReadAnimationFile m;
      Anki::Cozmo::ExternalInterface::MessageGameToEngine message;
      message.Set_ReadAnimationFile(m);
      // TODO: not thread safe maybe?
      s_externalInterface->Broadcast(message);
      
      // TODO: get from file and safe...
      Anki::Cozmo::ExternalInterface::PlayAnimation anim_m;
      anim_m.animationName = "MajorFail";
      Anki::Cozmo::ExternalInterface::MessageGameToEngine message2;
      message2.Set_PlayAnimation(anim_m);
      s_externalInterface->Broadcast(message2);
    }
    
    
    NSData *response = [@"<html><body>Correct<body></html>" dataUsingEncoding:NSUTF8StringEncoding];
    
    return [[HTTPDataResponse alloc] initWithData:response];
  }
  else if([method isEqualToString:@"GET"] )
  {
    NSData *get_response = [@"<html><body>GET RESPONSE<body></html>" dataUsingEncoding:NSUTF8StringEncoding];
    return [[HTTPDataResponse alloc] initWithData:get_response];
  }
  
  return [super httpResponseForMethod:method URI:path];
}

- (void)processBodyData:(NSData *)postDataChunk
{
  printf("processBodyData\n");
  
  // Remember: In order to support LARGE POST uploads, the data is read in chunks.
  // This prevents a 50 MB upload from being stored in RAM.
  // The size of the chunks are limited by the POST_CHUNKSIZE definition.
  // Therefore, this method may be called multiple times for the same POST request.
  
  BOOL result = [request appendData:postDataChunk];
  if (!result)
  {
    printf("Bad things happened?\n");
  }
}

@end

// TODO: clean up on destroy
// TODO: put all in #Shipping defines
HTTPServer* s_HttpServer = nil;

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
  /*UInt64 r1;
  NSString* contentLength = @"6";
  BOOL hasR1 = [NSNumber parseString:(NSString *)contentLength intoUInt64:&r1];
  if( hasR1)
  {
    printf("yay\n");
  }
  else
  {
    printf("nay\n");
  }*/
  
  
    NSError *err = nil;
    [s_HttpServer setPort:2223];
    
    [s_HttpServer setConnectionClass:[MyHTTPConnection class]];
    
    if (![s_HttpServer start:&err]) {
      //NSString* errorString = [NSString stringWithFormat:@"%@", err];
      //DASError("WifiConfigure.constructor", "Error starting http server: %s", [errorString UTF8String]);
      printf("Error starting \n");
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
void CreateUSBTunnelServer(Anki::Cozmo::IExternalInterface* externalInterface, Anki::Util::Data::DataPlatform* dataPlatform)
{
  s_dataPlatform = dataPlatform;
  s_externalInterface = externalInterface;
  //std::string full_path = _dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, name + "_PlayerName.wav");
  CreateUSBTunnelServerImpl();
}

#endif
