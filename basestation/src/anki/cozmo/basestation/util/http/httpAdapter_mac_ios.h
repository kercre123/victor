//
//  httpAdapter_osx.h
//  OverDrive
//
//  Created by Aubrey Goodman on 3/5/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "abstractHttpAdapter.h"
#import <Foundation/Foundation.h>

namespace Anki {
namespace Util {

  class HttpAdapter : public IHttpAdapter
  {
  public:

    explicit HttpAdapter();
    virtual ~HttpAdapter();

    void StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback callback);

    void ExecuteCallback(const uint64_t hash, const int responseCode, const std::map<std::string,std::string>& responseHeaders, const std::vector<uint8_t>& responseBody);
    
  private:

    NSURLSession* _urlSession;

    void ProcessGetRequest(NSString* httpMethod, const HttpRequest& request,
                           Util::Dispatch::Queue* queue, HttpRequestCallback callback);

    void ProcessUploadRequest(NSString* httpMethod,
                              const HttpRequest& request,
                              Util::Dispatch::Queue* queue,
                              HttpRequestCallback callback);

    NSMutableURLRequest* CreateURLRequest(const HttpRequest& request, NSString* httpMethod);

    void SetRequestHeaders(const HttpRequest& request, NSMutableURLRequest* urlRequest);

    void SetResponseHeaders(NSURLResponse* response, std::map<std::string,std::string>& responseHeaders);

    void ConvertParamsToBodyString(const HttpRequest& request, NSMutableString *bodyString);

    void DoCallback(Util::Dispatch::Queue* queue,
                    HttpRequestCallback callback,
                    const HttpRequest& request,
                    NSData* data,
                    NSURLResponse* response,
                    NSError* error);
  };
  
} // namespace Util
} // namespace Anki
