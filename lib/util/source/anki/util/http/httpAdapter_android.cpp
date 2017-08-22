//
//  httpAdapter_android.cpp
//  driveEngine
//
//  Created by Aubrey Goodman on 3/19/15.
//
//

#ifdef ANDROID

#include "util/http/httpAdapter_android.h"
#include "util/http/httpRequest.h"

#include "util/console/consoleInterface.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/helpers/ankiDefines.h"
#include "util/jni/jniUtils.h"
#include "util/logging/logging.h"

CONSOLE_VAR_RANGED(int, kHttpRequestTimeOutMSec, "Settings.Debug", 10000, 0, 100000);

namespace {
std::mutex instancesMutex;
std::unordered_map<uint64_t, Anki::Util::HttpAdapter*> requestInstances;
}

namespace Anki {
namespace Util {

std::atomic<uint64_t> HttpAdapter::_sHashCounter{1};

HttpAdapter::HttpAdapter(Util::Dispatch::Queue* queue /* = nullptr */)
: _internalQueue(queue)
, _adapterState(IHttpAdapter::AdapterState::DOWN)
{
  if (!ANKI_USE_JNI) {
    PRINT_NAMED_WARNING("http_adapter.disabled", "%s",
                        "JNI required but not available");
    return;
  }

  auto envWrapper = JNIUtils::getJNIEnvWrapper();
  JNIEnv* env = envWrapper->GetEnv();
  if (nullptr != env) {
    JClassHandle httpClass{env->FindClass("com/anki/util/http/HttpAdapter"), env};
    if (httpClass == nullptr) {
      PRINT_NAMED_ERROR("HttpAdapter_Android.Init.ClassNotFound", 
        "Unable to find com.anki.util.http.HttpAdapter");
      return;
    }

    {
      // get singleton instance of adapter
      jmethodID instanceMethodID = env->GetStaticMethodID(httpClass.get(), "getInstance", "()Lcom/anki/util/http/HttpAdapter;");
      JObjectHandle objInstance{env->CallStaticObjectMethod(httpClass.get(), instanceMethodID), env};

      // get global ref to class
      _javaObject = JGlobalObject{objInstance.get(), env};
      if (_javaObject == nullptr) {
        PRINT_NAMED_ERROR("HttpAdapter_Android.Init.ClassGlobalRef",
          "Unable to initialize global reference");
        return;
      }
    }

    // find StartRequest method
    _startRequestMethodID =
      env->GetMethodID(httpClass.get(), "startRequest",
        "(JLjava/lang/String;[Ljava/lang/String;[Ljava/lang/String;[BILjava/lang/String;I)V");
    if (_startRequestMethodID == nullptr) {
      PRINT_NAMED_ERROR("HttpAdapter_Android.Init.StartRequestMethodNotFound", 
        "Unable to find startRequest method");
      return;
    }
    SetAdapterState(IHttpAdapter::AdapterState::UP);
  }
  else {
    PRINT_NAMED_ERROR("HttpAdapter_Android.Init", "can't get JVM environment");
  }
}

HttpAdapter::~HttpAdapter()
{
  ShutdownAdapter();
}

void HttpAdapter::ShutdownAdapter()
{
  SetAdapterState(IHttpAdapter::AdapterState::DOWN);
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
                               Util::Dispatch::Queue* queue,
                               HttpRequestCallback callback,
                               HttpRequestDownloadProgressCallback progressCallback)
{
  if (!IsAdapterUp()) {
    return;
  }
  if (_internalQueue != nullptr) {
    Dispatch::Async(_internalQueue, [this, request = std::move(request), queue, callback = std::move(callback),
                                     progressCallback = std::move(progressCallback)] () mutable {
      StartRequestInternal(std::move(request), queue, std::move(callback), std::move(progressCallback));
    });
  }
  else {
    StartRequestInternal(std::move(request), queue, std::move(callback), std::move(progressCallback));
  }
}


void HttpAdapter::StartRequestInternal(HttpRequest&& request,
                                       Util::Dispatch::Queue* queue,
                                       HttpRequestCallback callback,
                                       HttpRequestDownloadProgressCallback progressCallback)
{
  if (!IsAdapterUp()) {
    return;
  }
  auto envWrapper = JNIUtils::getJNIEnvWrapper();
  JNIEnv* env = envWrapper->GetEnv();
  JNI_CHECK(env);
  if (_javaObject == nullptr || _startRequestMethodID == nullptr) {
    PRINT_NAMED_ERROR("HttpAdapter_Android.StartRequest.not_initialized",
      "Unable to initialize startRequest");
    return;
  }

  JStringHandle uri{env->NewStringUTF(request.uri.c_str()), env};

  // encode headers and params into array [k1,v1,k2,v2,...]
  std::vector<jstring> stringRefs;
  JObjectArrayHandle headers{JNIUtils::convertStringMapToJObjectArray(env, request.headers, stringRefs), env};
  JObjectArrayHandle params{JNIUtils::convertStringMapToJObjectArray(env, request.params, stringRefs), env};

  jint httpMethod = (jint)(request.method);
  jsize bodySize = (jsize)request.body.size();
  jint requestTimeout = (jint)request.timeOutMSec;

  JByteArrayHandle jbodyArray{env->NewByteArray(bodySize), env};
  env->SetByteArrayRegion(jbodyArray.get(), 0, bodySize, (const jbyte*)(request.body.data()));

  JStringHandle storageFilePath{env->NewStringUTF(request.storageFilePath.c_str()), env};

  PRINT_CH_INFO("HTTP", "http_adapter.request", "%s :: %s",
                       HttpMethodToString(request.method),
                       request.uri.c_str());

  uint64_t hash;
  {
    std::lock_guard<std::mutex> lock(_callbackMutex);
    hash = _sHashCounter.fetch_add(1);
    _requestResponseMap.emplace(hash,
      HttpRequestResponseType{.request = std::move(request),
        .queue = queue,
        .callback = std::move(callback),
        .progressCallback = std::move(progressCallback)});
  }
  // 'request' is now invalid
  {
    std::lock_guard<std::mutex> lock(instancesMutex);
    requestInstances.emplace(hash, this);
  }

  PRINT_NAMED_DEBUG("http_adapter.start_request.call_java", "");
  env->CallVoidMethod(_javaObject.get(), _startRequestMethodID, hash, uri.get(), headers.get(), params.get(),
                      jbodyArray.get(), httpMethod, storageFilePath.get(), requestTimeout);
  PRINT_NAMED_DEBUG("http_adapter.start_request.call_java_done", "");

  for (jstring& stringRef : stringRefs) {
    env->DeleteLocalRef(stringRef);
  }
}

void HttpAdapter::GetHttpRequestResponseTypeFromHash(const uint64_t hash,
                                                     HttpRequestResponseType& response)
{
  std::lock_guard<std::mutex> lock(_callbackMutex);
  response = _requestResponseMap.at(hash);
}


void HttpAdapter::ExecuteCallback(const uint64_t hash,
                                  const int responseCode,
                                  std::map<std::string,std::string>& responseHeaders,
                                  std::vector<uint8_t>& responseBody)
{
  if (!IsAdapterUp()) {
    return;
  }
  PRINT_NAMED_DEBUG("http_adapter.execute_callback", "%llx %d %d",
                    hash, responseCode, responseBody.size());
  HttpRequestResponseType response;
  GetHttpRequestResponseTypeFromHash(hash, response);
  PRINT_NAMED_DEBUG("http_adapter.execute_callback.response_found", "");

  PRINT_CH_INFO("HTTP", "http_adapter.response", "%d :: %s",
                       responseCode,
                       response.request.uri.c_str());

  Dispatch::Async(response.queue, [this, hash, response = std::move(response), responseCode,
                                   responseBody = std::move(responseBody),
                                   responseHeaders = std::move(responseHeaders)] {
    if (!IsAdapterUp()) {
      return;
    }
    response.callback(response.request, responseCode, responseHeaders, responseBody);

    PRINT_NAMED_DEBUG("http_adapter.execute_callback.callback_executed", "");
    {
      std::lock_guard<std::mutex> lock(_callbackMutex);
      _requestResponseMap.erase(hash);
      PRINT_NAMED_DEBUG("http_adapter.execute_callback.completed", "");
    }
    {
      std::lock_guard<std::mutex> lock(instancesMutex);
      requestInstances.erase(hash);
    }

  });
}

void HttpAdapter::ExecuteDownloadProgressCallback(const uint64_t hash,
                                                  const int64_t bytesWritten,
                                                  const int64_t totalBytesWritten,
                                                  const int64_t totalBytesExpectedToWrite)
{
  if (!IsAdapterUp()) {
    return;
  }
  HttpRequestResponseType response;
  GetHttpRequestResponseTypeFromHash(hash, response);

  if (response.progressCallback) {
    Dispatch::Async(response.queue,
      [this, response = std::move(response), bytesWritten, totalBytesWritten, totalBytesExpectedToWrite]() {
        if (!IsAdapterUp()) {
          return;
        }
        response.progressCallback(response.request, bytesWritten,
                                  totalBytesWritten, totalBytesExpectedToWrite);
    });
  }
}


}
}


extern "C"
{

void JNICALL
Java_com_anki_util_http_HttpAdapter_NativeHttpRequestCallback(JNIEnv* env, jobject thiz,
                                                                   jlong hash, jint responseCode,
                                                                   jobjectArray responseHeaders,
                                                                   jbyteArray responseBody)
{
  jbyte* bytes = env->GetByteArrayElements(responseBody, NULL);
  if( bytes != NULL ) {
    std::map<std::string,std::string> headers;
    jsize stringCount = env->GetArrayLength(responseHeaders);
    for (jsize i = 0; i < stringCount; i += 2) {
      Anki::Util::JStringHandle jKey{(jstring) env->GetObjectArrayElement(responseHeaders, i), env};
      Anki::Util::JStringHandle jValue{(jstring) env->GetObjectArrayElement(responseHeaders, i + 1), env};
      const char* key = env->GetStringUTFChars(jKey.get(), 0);
      const char* value = env->GetStringUTFChars(jValue.get(), 0);
      headers.emplace(key, value);
      env->ReleaseStringUTFChars(jKey.get(), key);
      env->ReleaseStringUTFChars(jValue.get(), value);
    }

    jsize bytesLen = env->GetArrayLength(responseBody);
    std::vector<uint8_t> body(bytes, bytes + bytesLen);
    env->ReleaseByteArrayElements(responseBody, bytes, JNI_ABORT);
    auto instanceIter = requestInstances.find(hash);
    if (instanceIter != requestInstances.end() && instanceIter->second->IsAdapterUp()) {
      instanceIter->second->ExecuteCallback(hash, responseCode, headers, body);
    }
  }
}

void JNICALL
Java_com_anki_util_http_HttpAdapter_NativeHttpRequestDownloadProgressCallback(JNIEnv* env,
                                                                  jobject thiz,
                                                                  jlong hash,
                                                                  jlong bytesWritten,
                                                                  jlong totalBytesWritten,
                                                                  jlong totalBytesExpectedToWrite)
{
  auto instanceIter = requestInstances.find(hash);
  if (instanceIter != requestInstances.end() && instanceIter->second->IsAdapterUp()) {
    instanceIter->second->ExecuteDownloadProgressCallback(hash, bytesWritten,
                                            totalBytesWritten, totalBytesExpectedToWrite);
  }
}

}

#endif // ANDROID
