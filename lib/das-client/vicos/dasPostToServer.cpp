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
#include <string>
#include <stdio.h>

#include <curl/curl.h>
#include <vector>

// Log options
// #define LOGD(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)
// #define LOGE(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

#define LOGD(fmt, ...) {}
#define LOGE(fmt, ...) {}

// Curl request options
// TO DO: VIC-3352 Enable CURLOPT_SSL_VERIFYPEER for DAS uploads
#define DAS_CURLOPT_SSL_VERIFYPEER 0
#define DAS_CURLOPT_VERBOSE 0

namespace {

//https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
std::string base64_encode(const std::string &in) {

    std::string out;

    int val=0, valb=-6;
    for (unsigned char c : in) {
        val = (val<<8) + c;
        valb += 8;
        while (valb>=0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
            valb-=6;
        }
    }
    if (valb>-6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
    while (out.size()%4) out.push_back('=');
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

  long site_response_code = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  CURL *curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, DAS_CURLOPT_SSL_VERIFYPEER);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, DAS_CURLOPT_VERBOSE);

    // Compose a request with keyword attributes required for DAS v2
    // TO DO: Enable gzip compression, update headers & encoding to match
    std::string postBody;
    postBody.append("Action=SendMessage");
    postBody.append("&MessageAttribute.1.Name=Content-Encoding");
    postBody.append("&MessageAttribute.1.Value.DataType=String");
    postBody.append("&MessageAttribute.1.Value.StringValue=base64");
    postBody.append("&MessageAttribute.2.Name=Content-Type");
    postBody.append("&MessageAttribute.2.Value.DataType=String");
    postBody.append("&MessageAttribute.2.Value.StringValue=application%2Fvnd.anki%2Bjson%3B+format%3Dnormal%3B+product%3Dvic");
    postBody.append("&MessageAttribute.3.Name=DAS-Transport-Version");
    postBody.append("&MessageAttribute.3.Value.DataType=Number");
    postBody.append("&MessageAttribute.3.Value.StringValue=2");

    const std::string & messageBody = base64_encode(postInfo);
    postBody.append("&MessageBody=");
    postBody.append(curl_easy_escape(curl, messageBody.c_str(), messageBody.size()));

    postBody.append("&Version=2012-11-05");

    LOGD("Sending: %s",  + postBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBody.c_str());

    curl_slist * headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded; charset=utf-8");
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
