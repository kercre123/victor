/**
 * File: httpAdapter_mac_ios.mm
 *
 * Created by Aubrey Goodman on 3/5/15.
 * Updated: Molly Jameson
 * Date:   2/4/16
 *
 * Description: Create native interface for http connections.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#import "anki/cozmo/basestation/util/http/httpAdapter_mac_ios.h"
#import "anki/cozmo/basestation/util/codeTimer.h"
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
        ProcessGetRequest(@"GET", request, queue, std::move(callback));
        break;
      case HttpMethodHead:
        ProcessGetRequest(@"HEAD", request, queue, std::move(callback));
        break;

      case HttpMethodPost:
        ProcessUploadRequest(@"POST", request, queue, std::move(callback));
        break;

      case HttpMethodPut:
        ProcessUploadRequest(@"PUT", request, queue, std::move(callback));
        break;

      case HttpMethodDelete:
        ProcessUploadRequest(@"DELETE", request, queue, std::move(callback));
        break;

      default:
        // this should never happen!
        assert(false);
        break;
    }
    
    PRINT_CH_INFO("HTTP", "http_adapter.request", "%s :: %s",
                         HttpMethodToString(request.method),
                         request.uri.c_str());

  }

  void HttpAdapter::ExecuteCallback(const uint64_t hash, const int responseCode, const std::map<std::string,std::string>& responseHeaders, const std::vector<uint8_t>& responseBody)
  {
    // unused; do nothing
  }

  void HttpAdapter::DoCallback(Util::Dispatch::Queue* queue,
                               HttpRequestCallback callback,
                               HttpRequest request,
                               NSData* data,
                               NSURLResponse* response,
                               NSError* error)
  {
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
    
    PRINT_CH_INFO("HTTP", "http_adapter.response", "%d :: %s",
                         responseCode,
                         request.uri.c_str());

    Util::Dispatch::Async(queue, [request, responseBody, responseCode, responseHeaders, callback] {
      callback(request, responseCode, responseHeaders, responseBody);
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
    NSURLSessionTask* task = nil;

    try {
      __block HttpRequest localRequestRef = request;
      NSMutableURLRequest* urlRequest = CreateURLRequest(request, httpMethod);

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
          DoCallback(queue, std::move(callback), std::move(localRequestRef), nil, response, error);

        }];
      } else {
        task = [_urlSession dataTaskWithRequest:urlRequest
                              completionHandler:^(NSData* data,
                                                  NSURLResponse* response,
                                                  NSError* error) {
          DoCallback(queue, std::move(callback), std::move(localRequestRef), data, response, error);
        }];
      }
    }
    catch (const std::exception& e) {
      PRINT_NAMED_WARNING("HttpAdapter.ProcessGetRequest", "exception: %s", e.what());
      return;
    }
    catch (...) {
      PRINT_NAMED_WARNING("HttpAdapter.ProcessGetRequest", "Objective-C exception?");
      return;
    }
    [task resume];
  }

  void HttpAdapter::ProcessUploadRequest(NSString* httpMethod,
                                         const HttpRequest& request,
                                         Util::Dispatch::Queue* queue,
                                         HttpRequestCallback callback)
  {
    try {
      __block HttpRequest localRequestRef = request;
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
        DoCallback(queue, std::move(callback), std::move(localRequestRef), data, response, error);
      }];
      [task resume];
    }
    catch (const std::exception& e) {
      PRINT_NAMED_WARNING("HttpAdapter.ProcessUploadRequest", "exception: %s", e.what());
      return;
    }
    catch (...) {
      PRINT_NAMED_WARNING("HttpAdapter.ProcessUploadRequest", "Objective-C exception?");
      return;
    }
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
