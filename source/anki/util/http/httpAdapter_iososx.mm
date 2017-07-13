//
//  httpAdapter_iososx.mm
//  OverDrive
//
//  Created by Aubrey Goodman on 3/5/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#import "httpAdapter_iososx.h"
#import "util/codeTimer/codeTimer.h"
#import "util/string/stringUtils.h"
#import "util/logging/logging.h"
#import "util/dispatchQueue/dispatchQueue.h"

@implementation HttpAdapterURLSessionDelegate
- (id) initWithAdapter:(Anki::Util::HttpAdapter*)adapter
{
  self = [super init];
  if (self) {
    m_Adapter = adapter;
  }
  return self;
}

- (void) clearAdapter
{
  m_Adapter = nullptr;
}

- (void) URLSession:(NSURLSession *)session didBecomeInvalidWithError:(NSError *)error
{
}

- (void) URLSession:(NSURLSession *)session
didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
  completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition,
                              NSURLCredential *credential))completionHandler
{
  completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, NULL);
}

- (void) URLSessionDidFinishEventsForBackgroundURLSession:(NSURLSession *)session
{
}

- (void) URLSession:(NSURLSession *)session
               task:(NSURLSessionTask *)task
didCompleteWithError:(NSError *)error
{
  if (m_Adapter) {
    m_Adapter->DidCompleteWithError(task, error);
  }
}

- (void) URLSession:(NSURLSession *)session
               task:(NSURLSessionTask *)task
didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
  completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition,
                              NSURLCredential *credential))completionHandler
{
  completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, NULL);
}

- (void) URLSession:(NSURLSession *)session
               task:(NSURLSessionTask *)task
    didSendBodyData:(int64_t)bytesSent
     totalBytesSent:(int64_t)totalBytesSent
totalBytesExpectedToSend:(int64_t)totalBytesExpectedToSend
{
}

- (void) URLSession:(NSURLSession *)session
               task:(NSURLSessionTask *)task
willPerformHTTPRedirection:(NSHTTPURLResponse *)response
         newRequest:(NSURLRequest *)request
  completionHandler:(void (^)(NSURLRequest *))completionHandler
{
  completionHandler(request);
}

- (void) URLSession:(NSURLSession *)session
       downloadTask:(NSURLSessionDownloadTask *)downloadTask
  didResumeAtOffset:(int64_t)fileOffset
 expectedTotalBytes:(int64_t)expectedTotalBytes
{

}

- (void) URLSession:(NSURLSession *)session
       downloadTask:(NSURLSessionDownloadTask *)downloadTask
       didWriteData:(int64_t)bytesWritten
  totalBytesWritten:(int64_t)totalBytesWritten
totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite
{
  if (m_Adapter) {
    m_Adapter->DidWriteData(downloadTask, bytesWritten,
                            totalBytesWritten, totalBytesExpectedToWrite);
  }
}

- (void) URLSession:(NSURLSession *)session
       downloadTask:(NSURLSessionDownloadTask *)downloadTask
didFinishDownloadingToURL:(NSURL *)location
{
  if (m_Adapter) {
    m_Adapter->DidFinishDownloadingToURL(downloadTask, location);
  }
}

@end


using namespace std;

namespace Anki {
namespace Util {

  HttpAdapter::HttpAdapter()
  {
    SetAdapterState(IHttpAdapter::AdapterState::UP);
    _urlSessionDelegate = [[HttpAdapterURLSessionDelegate alloc] initWithAdapter:this];

    _urlSession =
     [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]
                                   delegate:_urlSessionDelegate
                              delegateQueue:NULL];
  }

  HttpAdapter::~HttpAdapter()
  {
    ShutdownAdapter();
    if (_urlSessionDelegate) {
      [_urlSessionDelegate clearAdapter];
    }
    _urlSession = nil;
    _urlSessionDelegate = nil;
  }

  void HttpAdapter::ShutdownAdapter()
  {
    SetAdapterState(IHttpAdapter::AdapterState::DOWN);
    [_urlSession invalidateAndCancel];
  }

  bool HttpAdapter::IsAdapterUp()
  {
    return (IHttpAdapter::AdapterState::UP == GetAdapterState());
  }

  IHttpAdapter::AdapterState HttpAdapter::GetAdapterState()
  {
    std::lock_guard<std::mutex> lock(_adapterStateMutex);
    return _adapterState;
  }

  void HttpAdapter::SetAdapterState(const IHttpAdapter::AdapterState state)
  {
    std::lock_guard<std::mutex> lock(_adapterStateMutex);
    _adapterState = state;
  }

  void HttpAdapter::StartRequest(HttpRequest request,
                                 Dispatch::Queue* queue,
                                 HttpRequestCallback callback,
                                 HttpRequestDownloadProgressCallback progressCallback)
  {
    if (!IsAdapterUp()) {
      return;
    }

    PRINT_CH_INFO("HTTP", "http_adapter.request", "%s :: %s",
                  HttpMethodToString(request.method),
                  request.uri.c_str());

    switch (request.method) {
      case HttpMethodGet:
        ProcessGetRequest(@"GET", request, queue, std::move(callback), std::move(progressCallback));
        break;
      case HttpMethodHead:
        ProcessGetRequest(@"HEAD", request, queue, std::move(callback), std::move(progressCallback));
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
    // 'request' is no longer valid here
  }

  bool HttpAdapter::SetRequestStateForTask(NSURLSessionTask* task, HttpRequestState requestState)
  {
    std::lock_guard<std::mutex> lock(_taskToStateMapMutex);
    if (!task) {
      return false;
    }
    auto ret = _taskToStateMap.emplace(std::piecewise_construct,
                                       std::forward_as_tuple(task),
                                       std::forward_as_tuple(std::move(requestState)));
    return ret.second;
  }

  size_t HttpAdapter::EraseRequestStateForTask(NSURLSessionTask* task)
  {
    std::lock_guard<std::mutex> lock(_taskToStateMapMutex);
    return _taskToStateMap.erase(task);
  }

  bool HttpAdapter::GetRequestStateFromTask(NSURLSessionTask* task,
                                            HttpRequestState& requestState)
  {
    std::lock_guard<std::mutex> lock(_taskToStateMapMutex);
    if (!task) {
      return false;
    }
    auto search = _taskToStateMap.find(task);
    if (search == std::end(_taskToStateMap)) {
      return false;
    }
    requestState = search->second;
    return true;
  }

  void HttpAdapter::DidCompleteWithError(NSURLSessionTask* task,
                                         NSError* error)
  {
    if (!error) {
      return;
    }
    HttpRequestState requestState;
    if (!GetRequestStateFromTask(task, requestState)) {
      return;
    }
    NSHTTPURLResponse* response = (NSHTTPURLResponse *) task.response;
    DoCallback(requestState.queue,
               requestState.callback,
               requestState.request,
               nil,
               response,
               error);
    (void) EraseRequestStateForTask(task);
  }

  void HttpAdapter::DidFinishDownloadingToURL(NSURLSessionDownloadTask* downloadTask,
                                              NSURL* location)
  {
    HttpRequestState requestState;
    if (!GetRequestStateFromTask(downloadTask, requestState)) {
      return;
    }
    NSURL* destinationUrl = requestState.destinationUrl;
    NSHTTPURLResponse* response = (NSHTTPURLResponse *) downloadTask.response;
    int responseCode = (int) response.statusCode;
    NSError* error = downloadTask.error;
    if (!error && isHttpSuccessCode(responseCode)) {
      NSFileManager* fm = [[NSFileManager alloc] init];
      [fm removeItemAtURL:destinationUrl error:nil];
      [fm moveItemAtURL:location toURL:destinationUrl error:&error];
    }
    DoCallback(requestState.queue,
               requestState.callback,
               requestState.request,
               nil,
               response,
               error);
  (void) EraseRequestStateForTask(downloadTask);
  }

  void HttpAdapter::DidWriteData(NSURLSessionDownloadTask* downloadTask,
                                 int64_t bytesWritten,
                                 int64_t totalBytesWritten,
                                 int64_t totalBytesExpectedToWrite)
  {
    if (!IsAdapterUp()) {
      return;
    }
    HttpRequestState requestState;
    if (!GetRequestStateFromTask(downloadTask, requestState)) {
      return;
    }
    if (requestState.progressCallback) {
      Dispatch::Async(requestState.queue,
                            [this, requestState = std::move(requestState), bytesWritten,
                             totalBytesWritten, totalBytesExpectedToWrite] {
        if (!IsAdapterUp()) {
          return;
        }
        requestState.progressCallback(requestState.request,
                                      bytesWritten, totalBytesWritten, totalBytesExpectedToWrite);
      });
    }

  }

  static std::map<NSInteger, int> sNSErrorCodeToHttpStatusCode =
  {
    {NSURLErrorUserAuthenticationRequired, kHttpResponseCodeUnauthorized},
    {NSURLErrorNoPermissionsToReadFile, kHttpResponseCodeForbidden},
    {NSURLErrorUserAuthenticationRequired, kHttpResponseCodeProxyAuthenticationRequired}
  };

  int HttpAdapter::GetHttpStatusCodeForNSErrorCode(NSInteger errorCode) const
  {
    auto search = sNSErrorCodeToHttpStatusCode.find(errorCode);
    if (search == std::end(sNSErrorCodeToHttpStatusCode)) {
      return (int) errorCode;
    }
    return search->second;
  }


  void HttpAdapter::DoCallback(Dispatch::Queue* queue,
                               HttpRequestCallback callback,
                               HttpRequest request,
                               NSData* data,
                               NSURLResponse* response,
                               NSError* error)
  {
    if (!IsAdapterUp()) {
      return;
    }
    std::map<std::string,std::string> responseHeaders;
    SetResponseHeaders(response, responseHeaders);
    std::vector<uint8_t> responseBody;
    int responseCode;

    if (error) {
      responseCode = GetHttpStatusCodeForNSError(error);
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

    Dispatch::Async(queue, [this, responseCode,
                                  request = std::move(request),
                                  responseBody = std::move(responseBody),
                                  responseHeaders = std::move(responseHeaders),
                                  callback = std::move(callback)] {
      if (!IsAdapterUp()) {
        return;
      }
      callback(request, responseCode, responseHeaders, responseBody);
    });
  }

  void HttpAdapter::ProcessGetRequest(NSString* httpMethod, HttpRequest& request,
                                      Dispatch::Queue* queue, HttpRequestCallback callback,
                                      HttpRequestDownloadProgressCallback progressCallback)
  {
    NSMutableURLRequest* urlRequest = CreateURLRequest(request, httpMethod);
    [urlRequest setTimeoutInterval:(request.timeOutMSec/1000)];

    NSURLSessionTask* task = nil;

    if (!request.storageFilePath.empty()) {
      NSString* destinationPath =
      [NSString stringWithCString:request.storageFilePath.c_str()
                         encoding:[NSString defaultCStringEncoding]];
      NSURL* destinationUrl = [NSURL fileURLWithPath:destinationPath];
      task = [_urlSession downloadTaskWithRequest:urlRequest];
      HttpRequestState requestState(std::move(request), queue, callback,
                                    progressCallback, destinationUrl);
      (void) SetRequestStateForTask(task, std::move(requestState));
    } else {
      __block HttpRequest blockRequest = std::move(request);
      task = [_urlSession dataTaskWithRequest:urlRequest
                            completionHandler:^(NSData* data,
                                                NSURLResponse* response,
                                                NSError* error) {
        DoCallback(queue, callback, std::move(blockRequest), data, response, error);
      }];
    }
    [task resume];
  }

  void HttpAdapter::ProcessUploadRequest(NSString* httpMethod,
                                         HttpRequest& request,
                                         Dispatch::Queue* queue,
                                         HttpRequestCallback callback)
  {
    NSMutableURLRequest* urlRequest = CreateURLRequest(request, httpMethod);
    [urlRequest setTimeoutInterval:(request.timeOutMSec/1000)];

    const bool emptyBody = request.body.empty();
    const bool isFileRequest = emptyBody && !request.storageFilePath.empty();
    
    if (isFileRequest){
      NSString* filePath = [NSString stringWithCString:request.storageFilePath.c_str()
                                              encoding:NSUTF8StringEncoding];
      NSURL* url = [[NSURL alloc] initFileURLWithPath:filePath];
      
      __block HttpRequest blockRequest = std::move(request);
      auto completionHandler = ^(NSData* data, NSURLResponse* response, NSError* error) {
        DoCallback(queue, callback, std::move(blockRequest), data, response, error);
      };
      
      NSURLSessionDataTask *task = [_urlSession uploadTaskWithRequest:urlRequest
                                                             fromFile:url
                                                    completionHandler:completionHandler];
      [task resume];
    }else{
      NSMutableData* bodyData;
      if (emptyBody){
        // Generate & Set Parameter Data
        NSMutableString *bodyString = [NSMutableString string];
        ConvertParamsToBodyString(request, bodyString);
        bodyData = [[bodyString dataUsingEncoding:NSUTF8StringEncoding] copy];
      }else{
        bodyData = [[NSMutableData alloc] initWithCapacity:request.body.size()];
        [bodyData appendBytes:request.body.data() length:request.body.size()];
      }
      
      __block HttpRequest blockRequest = std::move(request);
      auto completionHandler = ^(NSData* data, NSURLResponse* response, NSError* error) {
        DoCallback(queue, callback, std::move(blockRequest), data, response, error);
      };
      
      NSURLSessionDataTask *task = [_urlSession uploadTaskWithRequest:urlRequest
                                                             fromData:bodyData
                                                    completionHandler:completionHandler];
      [task resume];
    }
  }

  NSMutableURLRequest* HttpAdapter::CreateURLRequest(const HttpRequest& request,
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

  void HttpAdapter::ConvertParamsToBodyString(const HttpRequest& request, NSMutableString *bodyString)
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
