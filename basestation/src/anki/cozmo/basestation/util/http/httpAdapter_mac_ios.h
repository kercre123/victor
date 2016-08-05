/**
 * File: httpAdapter_osx.h
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

#ifndef __Cozmo_Basestation_Util_Http_HttpAdapterMacIos_H_
#define __Cozmo_Basestation_Util_Http_HttpAdapterMacIos_H_

#include "util/http/abstractHttpAdapter.h"
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

#endif