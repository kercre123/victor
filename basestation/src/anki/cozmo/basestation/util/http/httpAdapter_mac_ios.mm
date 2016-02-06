//
//  httpAdapter_osx.mm
//  OverDrive
//
//  Created by Aubrey Goodman on 3/5/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#import "httpAdapter_mac_ios.h"
#import "../codeTimer.h"
#import "util/logging/logging.h"
#import "util/dispatchQueue/dispatchQueue.h"

using namespace std;

namespace Anki {
namespace Util {

  
  HttpAdapter::HttpAdapter()
  {
    _urlSession = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]];
  }

  HttpAdapter::~HttpAdapter()
  {
    _urlSession = nil;
  }

  void HttpAdapter::StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback callback)
  {
    switch (request.method) {
      case HttpMethodGet:
        ProcessGetRequest(@"GET", request, queue, callback);
        break;
      case HttpMethodHead:
        ProcessGetRequest(@"HEAD", request, queue, callback);
        break;

      case HttpMethodPost:
        ProcessUploadRequest(@"POST", request, queue, callback);
        break;

      case HttpMethodPut:
        ProcessUploadRequest(@"PUT", request, queue, callback);
        break;

      case HttpMethodDelete:
        ProcessUploadRequest(@"DELETE", request, queue, callback);
        break;

      default:
        // this should never happen!
        assert(false);
        break;
    }
    
    PRINT_CHANNELED_INFO("HTTP", "http_adapter.request", "%s :: %s",
                         HttpMethodToString(request.method),
                         request.uri.c_str());

  }

  void HttpAdapter::ExecuteCallback(const uint64_t hash, const int responseCode, const std::map<std::string,std::string>& responseHeaders, const std::vector<uint8_t>& responseBody)
  {
    // unused; do nothing
  }

  void HttpAdapter::DoCallback(Util::Dispatch::Queue* queue,
                               HttpRequestCallback callback,
                               const HttpRequest& request,
                               NSData* data,
                               NSURLResponse* response,
                               NSError* error)
  {
    HttpRequest localRequestRef = request;
    std::map<std::string,std::string> responseHeaders;
    SetResponseHeaders(response, responseHeaders);
    std::vector<uint8_t> responseBody;
    int responseCode;

    if (error) {
      responseCode = (int) error.code;
    } else {
      NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*) response;
      responseCode = (int) httpResponse.statusCode;

      if (data) {
        const uint8_t* bytes = (uint8_t*) data.bytes;
        responseBody.assign(bytes, bytes + data.length);
      }
    }
    
    PRINT_CHANNELED_INFO("HTTP", "http_adapter.response", "%d :: %s",
                         responseCode,
                         request.uri.c_str());

    Util::Dispatch::Async(queue, [localRequestRef, responseBody, responseCode, responseHeaders, callback] {
      callback(localRequestRef, responseCode, responseHeaders, responseBody);
      auto start = Util::CodeTimer::Start();
      
      int elapsedMillis = Util::CodeTimer::MillisecondsElapsed(start);
      if (elapsedMillis >= kHttpRequestCallbackTookTooLongMilliseconds)
      {
        // TODO: DAS delay
        // http_adapter.callback_took_too_long - data: duration in ms, sval: uri
        // Remove PII
        /*std::string piiRemovedString = AnkiUtil::RemovePII(localRequestRef.uri);
        
        Util::sEvent("http_adapter.callback_took_too_long",
                     {{DDATA, std::to_string(elapsedMillis).c_str()}},
                     piiRemovedString.c_str());*/
      }
    });
  }

  void HttpAdapter::ProcessGetRequest(NSString* httpMethod, const HttpRequest& request,
                                      Util::Dispatch::Queue* queue, HttpRequestCallback callback)
  {
    HttpRequest localRequestRef = request;
    NSMutableURLRequest* urlRequest = CreateURLRequest(request, httpMethod);

    NSURLSessionTask* task = nil;

    if (!request.destinationPath.empty()) {
      NSString* destinationPath =
      [NSString stringWithCString:request.destinationPath.c_str()
                         encoding:[NSString defaultCStringEncoding]];
      NSURL* destinationUrl = [NSURL fileURLWithPath:destinationPath];
      task = [_urlSession downloadTaskWithRequest:urlRequest
                                completionHandler:^(NSURL *location,
                                                    NSURLResponse *response,
                                                    NSError *error) {
        if (!error) {
          NSFileManager* fm = [[NSFileManager alloc] init];
          [fm moveItemAtURL:location toURL:destinationUrl error:&error];
        }
        DoCallback(queue, callback, localRequestRef, nil, response, error);

      }];
    } else {
      task = [_urlSession dataTaskWithRequest:urlRequest
                            completionHandler:^(NSData* data,
                                                NSURLResponse* response,
                                                NSError* error) {
        DoCallback(queue, callback, localRequestRef, data, response, error);
      }];
    }
    [task resume];
  }

  void HttpAdapter::ProcessUploadRequest(NSString* httpMethod,
                                         const HttpRequest& request,
                                         Util::Dispatch::Queue* queue,
                                         HttpRequestCallback callback)
  {
    HttpRequest localRequestRef = request;
    NSMutableURLRequest* urlRequest = CreateURLRequest(request, httpMethod);

    NSMutableData* bodyData;

    if (localRequestRef.body.empty()){
      // Generate & Set Parameter Data
      NSMutableString *bodyString = [NSMutableString string];
      ConvertParamsToBodyString(localRequestRef, bodyString);
      bodyData = [[bodyString dataUsingEncoding:NSUTF8StringEncoding] copy];
    }else{
      bodyData = [[NSMutableData alloc] initWithCapacity:localRequestRef.body.size()];
      [bodyData appendBytes:localRequestRef.body.data() length:localRequestRef.body.size()];
    }

    NSURLSessionDataTask *task = [_urlSession uploadTaskWithRequest:urlRequest
                                                           fromData:bodyData
                                                  completionHandler:^(NSData* data,
                                                                      NSURLResponse* response,
                                                                      NSError* error) {
      DoCallback(queue, callback, localRequestRef, data, response, error);
    }];
    [task resume];
  }

  NSMutableURLRequest* HttpAdapter::CreateURLRequest(const Anki::Util::HttpRequest& request,
                                                     NSString* httpMethod)
  {
    NSURL* resourceURL =
      [NSURL URLWithString:[NSString stringWithCString:request.uri.c_str()
                                              encoding:NSUTF8StringEncoding]];

    NSMutableURLRequest* urlRequest = [NSMutableURLRequest requestWithURL:resourceURL];
    urlRequest.HTTPMethod = httpMethod;
    SetRequestHeaders(request, urlRequest);

    return urlRequest;
  }

  void HttpAdapter::SetRequestHeaders(const HttpRequest& request, NSMutableURLRequest* urlRequest)
  {
    for (auto it=request.headers.begin(); it!=request.headers.end(); ++it) {
      [urlRequest addValue:[NSString stringWithCString:it->second.c_str() encoding:NSUTF8StringEncoding] forHTTPHeaderField:[NSString stringWithCString:it->first.c_str() encoding:NSUTF8StringEncoding]];
    }
  }

  void HttpAdapter::SetResponseHeaders(NSURLResponse* response, map<string, string>& responseHeaders)
  {
    if( [response respondsToSelector:@selector(allHeaderFields)] ) {
      NSDictionary* headers = [(NSHTTPURLResponse*)response allHeaderFields];
      for (NSString* key in headers.allKeys) {
        NSString* val = headers[key];
        responseHeaders.insert( pair<string,string>(key.UTF8String,val.UTF8String) );
      }
    }
  }

  void HttpAdapter::ConvertParamsToBodyString(const Anki::Util::HttpRequest& request, NSMutableString *bodyString)
  {
    bool first = true;
    for (auto it=request.params.begin(); it!=request.params.end(); ++it) {
      if (first) {
        first = false;
      }else{
        [bodyString appendString:@"&"];
      }
      NSString *key = [NSString stringWithCString:it->first.c_str() encoding:NSUTF8StringEncoding];
      NSString *value = [NSString stringWithCString:it->second.c_str() encoding:NSUTF8StringEncoding];
      [bodyString appendFormat:@"%@=%@", key, value];
    }
  }

} // namespace DriveEngine
} // namespace Anki
