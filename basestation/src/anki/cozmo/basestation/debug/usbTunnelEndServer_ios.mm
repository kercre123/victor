
#include "anki/cozmo/basestation/debug/usbTunnelEndServer_ios.h"

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
  if ([method isEqualToString:@"POST"] )
  {
   //Check for command that says update and play an animation
    //localhost:2223/cmd_anim_update/anim_name
    if( [path containsString:@"cmd_anim_update/"] )
    {
      NSData *postData = [request body];
      if (postData)
      {
        NSString* postStr = [[NSString alloc] initWithData:postData encoding:NSUTF8StringEncoding];
        NSArray* path_list = [path componentsSeparatedByString:@"/"];
        if( path_list && [path_list count] > 2)
        {
          // strip name from path
          //std::string anim_name = "MajorFail";
          std::string anim_name = [[path_list objectAtIndex:2] UTF8String];
          // TODO: delete temp version on shutdown...
          
          // The "resource" file is not directly writable on device, so write to documents and then let reload overwrite.
          std::string full_path = s_dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, "TestAnim.json");
          std::string content = [postStr UTF8String];
          Anki::Util::FileUtils::WriteFile(full_path, content);
          
          
          Anki::Cozmo::ExternalInterface::ReadAnimationFile m;
          Anki::Cozmo::ExternalInterface::MessageGameToEngine message;
          message.Set_ReadAnimationFile(m);
          // TODO: not thread safe maybe?
          s_externalInterface->Broadcast(message);
          
          // TODO: get from file and safe...
          Anki::Cozmo::ExternalInterface::PlayAnimation anim_m;
          anim_m.animationName = anim_name;
          anim_m.numLoops = 1;
          Anki::Cozmo::ExternalInterface::MessageGameToEngine message2;
          message2.Set_PlayAnimation(anim_m);
          s_externalInterface->Broadcast(message2);
        }
      }
    }
    
    NSData *response = [@"Cozmo USB tunnel: Post Recieved\n" dataUsingEncoding:NSUTF8StringEncoding];
    return [[HTTPDataResponse alloc] initWithData:response];
  }
  else if([method isEqualToString:@"GET"] )
  {
    NSData *get_response = [@"<html><body>Cozmo USB tunnel: Get Recieved<body></html>\n" dataUsingEncoding:NSUTF8StringEncoding];
    return [[HTTPDataResponse alloc] initWithData:get_response];
  }
  
  return [super httpResponseForMethod:method URI:path];
}

- (void)processBodyData:(NSData *)postDataChunk
{
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

  
    NSError *err = nil;
    [s_HttpServer setPort:2223];
    
    [s_HttpServer setConnectionClass:[MyHTTPConnection class]];
   
    if (![s_HttpServer start:&err]) {
      printf("Error starting \n");
      [s_HttpServer stop];
      s_HttpServer = nil;
    }
  //} // autoreleasepool
}
void CreateUSBTunnelServer(Anki::Cozmo::IExternalInterface* externalInterface, Anki::Util::Data::DataPlatform* dataPlatform)
{
  s_dataPlatform = dataPlatform;
  s_externalInterface = externalInterface;
  CreateUSBTunnelServerImpl();
}

#endif
