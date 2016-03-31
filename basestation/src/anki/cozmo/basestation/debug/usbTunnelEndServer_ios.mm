/**
 * File: usbTunnelEndServer.cpp
 *
 * Author: Molly Jameson
 * Created: 03/28/16
 *
 * Description: Handles the ios specific server function for usb tunnel
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/debug/usbTunnelEndServer_ios.h"

#ifdef ANKI_DEV_CHEATS

#if __APPLE__
#include "TargetConditionals.h"
#endif

#if TARGET_OS_IPHONE
#import <Foundation/Foundation.h>

#import "HTTPConnection.h"
#import "HTTPDataResponse.h"
#import "HTTPMessage.h"
#import "HTTPServer.h"

#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/fileUtils/fileUtils.h"


@interface CozmoUSBTunnelHTTPServer : HTTPServer
{
  Anki::Util::Data::DataPlatform* _dataPlatform;
  Anki::Cozmo::IExternalInterface* _externalInterface;
}
@end

@implementation CozmoUSBTunnelHTTPServer
- (Anki::Util::Data::DataPlatform*)GetDataPlatform
{
  return _dataPlatform;
}
- (void)SetDataPlatform:(Anki::Util::Data::DataPlatform*)dp
{
  _dataPlatform = dp;
}

- (Anki::Cozmo::IExternalInterface*)GetExternalInterface
{
  return _externalInterface;
}
- (void)SetExternalInterface:(Anki::Cozmo::IExternalInterface*)ei
{
  _externalInterface = ei;
}
@end


@interface CozmoUSBTunnelHTTPConnection : HTTPConnection
// Just a bunch of funcitonal overrides
@end


@implementation CozmoUSBTunnelHTTPConnection

- (BOOL)supportsMethod:(NSString *)method atPath:(NSString *)path
{
  if ([method isEqualToString:@"POST"])
  {
    // If we want to be careful with size check it here with requestContentLength
    return YES;
  }
  if ([method isEqualToString:@"GET"])
  {
    return YES;
  }
  return [super supportsMethod:method atPath:path];
}

- (BOOL)expectsRequestBodyFromMethod:(NSString *)method atPath:(NSString *)path
{
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
    // In the future we could have a command for updating other things via server command line as well.
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
          std::string anim_name = [[path_list objectAtIndex:2] UTF8String];
          
          CozmoUSBTunnelHTTPServer* cozmo_server = (CozmoUSBTunnelHTTPServer*)[config server];
          Anki::Util::Data::DataPlatform* data_platform = [cozmo_server GetDataPlatform];
          Anki::Cozmo::IExternalInterface* external_interface = [cozmo_server GetExternalInterface];
          if( data_platform && external_interface)
          {
            // The "resource" file is not directly writable on device, so write to documents and then let reload overwrite.
            std::string full_path = data_platform->pathToResource(Anki::Util::Data::Scope::Cache, Anki::Cozmo::USBTunnelServer::TempAnimFileName);
            std::string content = [postStr UTF8String];
            Anki::Util::FileUtils::WriteFile(full_path, content);
            
            Anki::Cozmo::ExternalInterface::ReadAnimationFile m;
            Anki::Cozmo::ExternalInterface::MessageGameToEngine message;
            message.Set_ReadAnimationFile(m);
            // TODO: not thread safe maybe?
            external_interface->Broadcast(message);
            
            // TODO: get from file and safe...
            Anki::Cozmo::ExternalInterface::PlayAnimation anim_m;
            anim_m.animationName = anim_name;
            anim_m.numLoops = 1;
            Anki::Cozmo::ExternalInterface::MessageGameToEngine message2;
            message2.Set_PlayAnimation(anim_m);
            external_interface->Broadcast(message2);
          }
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
  [request appendData:postDataChunk];
}

@end

namespace Anki {
  namespace Cozmo {
    
    // lightweight wrapper inside of other lightweight wrapper
    struct USBTunnelServer::USBTunnelServerImpl {
      CozmoUSBTunnelHTTPServer* _httpServer = nil;
    };
    
    const std::string USBTunnelServer::TempAnimFileName("TestAnim.json");
    
    USBTunnelServer::USBTunnelServer(Anki::Cozmo::IExternalInterface* externalInterface, Anki::Util::Data::DataPlatform* dataPlatform) : _impl(new USBTunnelServerImpl{})
    {
      
      @autoreleasepool {
        _impl->_httpServer = [[CozmoUSBTunnelHTTPServer alloc] init];
        [_impl->_httpServer setType:@"_http._tcp."];
        
        [_impl->_httpServer SetDataPlatform:dataPlatform];
        [_impl->_httpServer SetExternalInterface:externalInterface];
        // The maya script talks to this.
        [_impl->_httpServer setPort:2223];
        // Custom class that has a bunch of our overrides
        [_impl->_httpServer setConnectionClass:[CozmoUSBTunnelHTTPConnection class]];
        
        NSError *err = nil;
        if (![_impl->_httpServer start:&err]) {
          [_impl->_httpServer stop];
          _impl->_httpServer = nil;
        }
      }
    }
    
    USBTunnelServer::~USBTunnelServer()
    {
      // Stop the server
      if (_impl->_httpServer != nil) {
         Anki::Util::Data::DataPlatform* dataPlatform = [_impl->_httpServer GetDataPlatform];
        // Clean up what we might have created
        std::string full_path = dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, USBTunnelServer::TempAnimFileName);
        if( Util::FileUtils::FileExists(full_path))
        {
          Util::FileUtils::DeleteFile(full_path);
        }
        [_impl->_httpServer stop];
        _impl->_httpServer = nil;
      }
    }
  }
}
#else // not on iphone, just mac build
USBTunnelServer::USBTunnelServer(Anki::Cozmo::IExternalInterface* , Anki::Util::Data::DataPlatform* ) {}
USBTunnelServer::~USBTunnelServer() {}
#endif // target iOS

#endif // shipping