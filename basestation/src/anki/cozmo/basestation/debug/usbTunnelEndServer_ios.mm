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
#include "anki/cozmo/basestation/animations/animationTransfer.h"

#if ANKI_DEV_CHEATS

#if __APPLE__
#include "TargetConditionals.h" // must be included for TARGET_OS_IPHONE
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
// Just a bunch of functional overrides
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
  if ([method isEqualToString:@"POST"])
    return YES;
  
  return [super expectsRequestBodyFromMethod:method atPath:path];
}

- (BOOL)prepareFaceAnimDir:(NSString *)path
{
  NSArray* path_list = [path componentsSeparatedByString:@"/"];
  if (path_list && [path_list count] > 2)
  {
    // strip name from path
    std::string anim_name = [[path_list objectAtIndex:2] UTF8String];

    CozmoUSBTunnelHTTPServer* cozmo_server = (CozmoUSBTunnelHTTPServer*)[config server];
    Anki::Util::Data::DataPlatform* data_platform = [cozmo_server GetDataPlatform];
    if (data_platform)
    {
      // The "resource" file is not directly writable on device, so write to documents and then let reload overwrite.
      std::string face_anim_dir = data_platform->pathToResource(Anki::Util::Data::Scope::Cache,
                                                                Anki::Cozmo::USBTunnelServer::FaceAnimsDir);
      std::string full_path = Anki::Util::FileUtils::FullFilePath({face_anim_dir, anim_name});
      if (Anki::Util::FileUtils::DirectoryExists(full_path))
      {
        Anki::Util::FileUtils::RemoveDirectory(full_path);
      }
      Anki::Util::FileUtils::CreateDirectory(full_path);
      return YES;
    }
  }
  return NO;
}

- (BOOL)doFaceAnimUpdate:(NSString *)path
{
  // TODO: Change this to receive binary data and then "CopyFile()" instead of "WriteFile()" ???
  NSData *postData = [request body];
  if (postData)
  {
    NSArray* path_list = [path componentsSeparatedByString:@"/"];
    if (path_list && [path_list count] > 3)
    {
      // strip names from path
      std::string anim_name = [[path_list objectAtIndex:2] UTF8String];
      std::string file_name = [[path_list objectAtIndex:3] UTF8String];

      CozmoUSBTunnelHTTPServer* cozmo_server = (CozmoUSBTunnelHTTPServer*)[config server];
      Anki::Util::Data::DataPlatform* data_platform = [cozmo_server GetDataPlatform];
      if (data_platform)
      {
        // The "resource" file is not directly writable on device, so write to documents and then let reload overwrite.
        std::string face_anim_dir = data_platform->pathToResource(Anki::Util::Data::Scope::Cache,
                                                                  Anki::Cozmo::USBTunnelServer::FaceAnimsDir);
        std::string dir_path = Anki::Util::FileUtils::FullFilePath({face_anim_dir, anim_name});
        std::string file_path = Anki::Util::FileUtils::FullFilePath({dir_path, file_name});

        // Can this be more efficient?
        NSUInteger length = postData.length;
        uint8_t *bytes = (uint8_t *)[postData bytes];
        std::vector<uint8_t> v(bytes, bytes + length);
        Anki::Util::FileUtils::WriteFile(file_path, v);

        return YES;
      }
    }
  }
  return NO;
}

- (BOOL)processFaceAnimUpdate:(NSString *)path
{
  NSData *postData = [request body];
  if (postData)
  {
    CozmoUSBTunnelHTTPServer* cozmo_server = (CozmoUSBTunnelHTTPServer*)[config server];
    Anki::Cozmo::IExternalInterface* external_interface = [cozmo_server GetExternalInterface];
    if (external_interface)
    {
      Anki::Cozmo::ExternalInterface::MessageGameToEngine read_msg;
      Anki::Cozmo::ExternalInterface::ReadFaceAnimationDir m;
      read_msg.Set_ReadFaceAnimationDir(m);
      external_interface->BroadcastDeferred(std::move(read_msg));
      return YES;
    }
  }
  return NO;
}

- (BOOL)doAnimUpdate:(NSString *)path
{
  NSData *postData = [request body];
  if (postData)
  {
    NSString* postStr = [[NSString alloc] initWithData:postData encoding:NSUTF8StringEncoding];
    NSArray* path_list = [path componentsSeparatedByString:@"/"];
    if (path_list && [path_list count] > 2)
    {
      // strip name from path
      std::string anim_name = [[path_list objectAtIndex:2] UTF8String];
      
      CozmoUSBTunnelHTTPServer* cozmo_server = (CozmoUSBTunnelHTTPServer*)[config server];
      Anki::Util::Data::DataPlatform* data_platform = [cozmo_server GetDataPlatform];
      Anki::Cozmo::IExternalInterface* external_interface = [cozmo_server GetExternalInterface];
      if (data_platform && external_interface)
      {
        
        // The "resource" file is not directly writable on device, so write to documents and then let reload overwrite.
        std::string full_path = data_platform->pathToResource(Anki::Util::Data::Scope::Cache, Anki::Cozmo::USBTunnelServer::TempAnimFileName);
        std::string content = [postStr UTF8String];
        Anki::Util::FileUtils::WriteFile(full_path, content);
        
        // Since we're on our own thread make sure to call ThreadSafe Version.
        Anki::Cozmo::ExternalInterface::MessageGameToEngine read_msg;
        Anki::Cozmo::ExternalInterface::ReadAnimationFile m;
        read_msg.Set_ReadAnimationFile(m);
        external_interface->BroadcastDeferred(std::move(read_msg));
        
        Anki::Cozmo::ExternalInterface::MessageGameToEngine play_msg;
        Anki::Cozmo::ExternalInterface::PlayAnimation play_msg_content;
        play_msg_content.animationName = anim_name;
        play_msg_content.numLoops = 1;
        play_msg.Set_PlayAnimation(std::move(play_msg_content));
        external_interface->BroadcastDeferred(std::move(play_msg));
        return YES;
      }
    }
  }
  return NO;
}

- (BOOL)disableReactionGroup:(NSString *)path
{
  CozmoUSBTunnelHTTPServer* cozmo_server = (CozmoUSBTunnelHTTPServer*)[config server];
  Anki::Cozmo::IExternalInterface* external_interface = [cozmo_server GetExternalInterface];
  if (external_interface)
  {
    bool is_enabled = [path containsString:@"true"];
    Anki::Cozmo::ExternalInterface::MessageGameToEngine reaction_enable_msg;
    Anki::Cozmo::ExternalInterface::EnableReactionaryBehaviors reaction_enable_content(is_enabled);
    reaction_enable_msg.Set_EnableReactionaryBehaviors(std::move(reaction_enable_content));
    external_interface->BroadcastDeferred(std::move(reaction_enable_msg));
    return YES;
  }
  return NO;
}

- (NSObject<HTTPResponse> *)httpResponseForMethod:(NSString *)method URI:(NSString *)path
{
  @autoreleasepool
  {
    if ( [method isEqualToString:@"POST"] )
    {
      //Check for command that says update and play an animation
      // In the future we could have a command for updating other things via server command line as well.
      //localhost:2223/cmd_anim_update/anim_name
      if( [path containsString:@"cmd_anim_update/"] )
      {
        [self doAnimUpdate:path];
      }
      else if( [path containsString:@"cmd_face_anim_setup/"] )
      {
        [self prepareFaceAnimDir:path];
      }
      else if( [path containsString:@"cmd_face_anim_install/"] )
      {
        [self doFaceAnimUpdate:path];
      }
      else if( [path containsString:@"cmd_face_anim_refresh/"] )
      {
        [self processFaceAnimUpdate:path];
      }
      else if( [path containsString:@"cmd_enable_reactions/"] )
      {
        [self disableReactionGroup:path];
      }
      
      NSData *response = [@"Cozmo USB tunnel: Post Received\n" dataUsingEncoding:NSUTF8StringEncoding];
      return [[HTTPDataResponse alloc] initWithData:response];
    }
    else if( [method isEqualToString:@"GET"] )
    {
      NSData *get_response = [@"<html><body>Cozmo USB tunnel: Get Received<body></html>\n" dataUsingEncoding:NSUTF8StringEncoding];
      return [[HTTPDataResponse alloc] initWithData:get_response];
    }
    
    return [super httpResponseForMethod:method URI:path];
  } // @autoreleasepool
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
    
    const std::string USBTunnelServer::TempAnimFileName(AnimationTransfer::kCacheAnimFileName);
    const std::string USBTunnelServer::FaceAnimsDir(AnimationTransfer::kCacheFaceAnimsDir);
    
    USBTunnelServer::USBTunnelServer(Anki::Cozmo::IExternalInterface* externalInterface, Anki::Util::Data::DataPlatform* dataPlatform) : _impl(new USBTunnelServerImpl{})
    {
      
      @autoreleasepool {
        _impl->_httpServer = [[CozmoUSBTunnelHTTPServer alloc] init];
        [_impl->_httpServer setType:@"_http._tcp."];
        
        [_impl->_httpServer SetDataPlatform:dataPlatform];
        [_impl->_httpServer SetExternalInterface:externalInterface];
        // The maya script talks to this port, must be known
        [_impl->_httpServer setPort: (TARGET_OS_IPHONE) ? 2223 :8080];
        // Custom class that has a bunch of our overrides
        [_impl->_httpServer setConnectionClass:[CozmoUSBTunnelHTTPConnection class]];
        
        NSError *err = nil;
        if (![_impl->_httpServer start:&err])
        {
          [_impl->_httpServer stop];
          _impl->_httpServer = nil;
          NSString* errorString = [NSString stringWithFormat:@"%@", err];
          PRINT_NAMED_ERROR("USBTunnelServer.constructor","%s",[errorString UTF8String]);
        }
      }
    }
    
    USBTunnelServer::~USBTunnelServer()
    {
      // Stop the server
      if (_impl->_httpServer != nil)
      {
         Anki::Util::Data::DataPlatform* dataPlatform = [_impl->_httpServer GetDataPlatform];
        // Clean up what we might have created
        std::string full_path = dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, USBTunnelServer::TempAnimFileName);
        if( Util::FileUtils::FileExists(full_path))
        {
          Util::FileUtils::DeleteFile(full_path);
        }
        std::string face_anim_dir = dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, USBTunnelServer::FaceAnimsDir);
        if (Util::FileUtils::DirectoryExists(face_anim_dir))
        {
          Util::FileUtils::RemoveDirectory(face_anim_dir);
        }
        [_impl->_httpServer stop];
        _impl->_httpServer = nil;
      }
    }
  }
}
#else // not apple
#include <string>
namespace Anki {
  namespace Cozmo {
      const std::string USBTunnelServer::TempAnimFileName("TestAnim.json");
      const std::string USBTunnelServer::FaceAnimsDir(Anki::Util::FileUtils::FullFilePath({"assets", "faceAnimations"}));
      struct USBTunnelServer::USBTunnelServerImpl {};
      USBTunnelServer::USBTunnelServer(Anki::Cozmo::IExternalInterface* , Anki::Util::Data::DataPlatform* ) {}
      USBTunnelServer::~USBTunnelServer() {}
  }
}
#endif // target iOS

#endif // shipping
