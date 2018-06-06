/**
 * File: das-client/vicos/dasPostToServer.cpp
 *
 * Description: Function to be implemented on a per platform basis
 *              to do an HTTP POST to a given URL with the supplied
 *              postBody
 *
 * Copyright: Anki, Inc. 2015-2018
 *
 **/

#include "dasPostToServer.h"

#include <zlib.h>
#include <string>
#include <stdio.h>

#include <curl/curl.h>
#include <vector>

// Log options
#define ENABLE_LOGE 1
#define ENABLE_LOGD 0

#if ENABLE_LOGE
# define LOGE(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#else
# define LOGE(fmt, ...) {}
#endif

#if ENABLE_LOGD
# define LOGD(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)
#else
# define LOGD(fmt, ...) {}
#endif


// Curl request options
// TO DO: VIC-3352 Enable CURLOPT_SSL_VERIFYPEER for DAS uploads
#define DAS_CURLOPT_SSL_VERIFYPEER 0
#define DAS_CURLOPT_VERBOSE 0

namespace {

//https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
std::string base64_encode(const unsigned char * in, unsigned long inlen)
{
  std::string out;
  int val=0, valb=-6;
  while (inlen > 0) {
    val = (val<<8) + (*in);
    valb += 8;
    while (valb>=0) {
      out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
      valb-=6;
    }
    ++in, --inlen;
  }
  if (valb>-6) {
    out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
  }
  while (out.size()%4) {
    out.push_back('=');
  }
  return out;
}

} // end anonymous namespace

#ifdef __cplusplus
extern "C" {
#endif

size_t curl_write_function(char *ptr, size_t size, size_t nmemb, std::string *userdata)
{
  userdata->append(ptr, size*nmemb);
  return size*nmemb;
}

bool dasPostToServer(const std::string& url, const std::string& postInfo, std::string& out_response)
{
  LOGD("dasPostToServer: URL=%s", url.c_str());
  LOGD("dasPostToServer: BODY=%s", postInfo.c_str());

  // Compress payload into temporary buffer, then encode into temporary string
  // http://www.zlib.net/manual.html#Advanced

  // Initialize zlib parameters
  constexpr auto zLevel = Z_DEFAULT_COMPRESSION;
  constexpr auto zMethod = Z_DEFLATED;
  constexpr auto zWindowSize = 9+16;
  constexpr auto zMemLevel = 9;
  constexpr auto zStrategy = Z_DEFAULT_STRATEGY;

  // Initialize zlib stream struct
  z_stream stream = {0};
  stream.next_in = (Bytef *) postInfo.c_str();
  stream.avail_in = (uInt) postInfo.size();

  int rv = deflateInit2(&stream, zLevel, zMethod, zWindowSize, zMemLevel, zStrategy);
  if (rv != Z_OK) {
    LOGE("dasPostToServer.deflateInit2: Unable to initialize stream (error %d)", rv);
    return false;
  }

  // Initialize zlib output buffer. How much space do we need?
  const auto dst_bound = deflateBound(&stream, stream.avail_in);
  std::auto_ptr<unsigned char> dst(new unsigned char[dst_bound]);
  stream.next_out = dst.get();
  stream.avail_out = dst_bound;

  // Initialize gzip file header
  gz_header header = {0};
  rv = deflateSetHeader(&stream, &header);
  if (rv != Z_OK) {
    LOGE("dasPostToServer.deflateSetHeader: Unable to initialize header (error %d)", rv);
    return false;
  }

  // Perform the compression
  rv = deflate(&stream, Z_FINISH);
  if (rv != Z_STREAM_END) {
    LOGE("dasPostToServer.deflate: Unable to deflate (error %d)", rv);
    return false;
  }

  // Add any footer
  rv = deflateEnd(&stream);
  if (rv != Z_OK) {
    LOGE("dasPostToServer.deflateEnd: Unable to finish deflate (error %d)", rv);
    return false;
  }

  const size_t totalOut = stream.total_out;

  LOGD("dasPostToServer: compressed %zu to %zu", postInfo.size(), totalOut);

  long site_response_code = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  CURL *curl = curl_easy_init();
  if (curl) {
    // Add base64 and URL encoding to messageBody
    const std::string & encodedBody = base64_encode(dst.get(), totalOut);
    LOGD("dasPostToServer: encoded body = %s", encodedBody.c_str());
    const std::string & escapedBody = curl_easy_escape(curl, encodedBody.c_str(), encodedBody.size());
    LOGD("dasPostToServer: escaped body = %s", escapedBody.c_str());

    // Set up curl options for the request
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, DAS_CURLOPT_SSL_VERIFYPEER);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, DAS_CURLOPT_VERBOSE);

    // Compose a request with keyword attributes required for DAS v2
    std::string postBody;
    postBody.append("Action=SendMessage");
    postBody.append("&MessageAttribute.1.Name=DAS-Transport-Version");
    postBody.append("&MessageAttribute.1.Value.DataType=Number");
    postBody.append("&MessageAttribute.1.Value.StringValue=2");
    postBody.append("&MessageAttribute.2.Name=Content-Encoding");
    postBody.append("&MessageAttribute.2.Value.DataType=String");
    postBody.append("&MessageAttribute.2.Value.StringValue=gzip%2C+base64");
    postBody.append("&MessageAttribute.3.Name=Content-Type");
    postBody.append("&MessageAttribute.3.Value.DataType=String");
    postBody.append("&MessageAttribute.3.Value.StringValue=application%2Fvnd.anki%2Bjson%3B+format%3Dnormal%3B+product%3Dvic");
    postBody.append("&MessageBody=");
    postBody.append(escapedBody);
    postBody.append("&Version=2012-11-05");

    LOGD("Sending: %s",  + postBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBody.c_str());

    curl_slist * headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded; charset=utf-8");
    headers = curl_slist_append(headers, "Accept-Encoding: gzip");
    std::string lengthHeader("Content-Length: ");
    lengthHeader += std::to_string((unsigned long)postBody.size());
    headers = curl_slist_append(headers, lengthHeader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Set up a callback to receive response
    std::string response_info;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_info);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_function);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      const char * strerr = curl_easy_strerror(res);
      LOGE("curl_easy_perform() failed: %s", strerr);
      out_response.append(strerr);
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &site_response_code);
    LOGD("Response code: %ld", site_response_code);
    out_response.append(response_info);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
  }
  curl_global_cleanup();

  if (site_response_code == 200) {
    return true;
  }

  return false;
}

#ifdef __cplusplus
}
#endif
