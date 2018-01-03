/**
 * File: dasPostToServer.cpp
 *
 * Author: seichert
 * Created: 01/15/2015
 *
 * Description: Function to be implemented on a per platform basis
 *              to do an HTTP POST to a given URL with the supplied
 *              postBody
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "dasPostToServer.h"
#include <string>
#include <stdio.h>

#if defined(INTEGRATION_TEST)
#include <curl/curl.h>
#include <vector>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(INTEGRATION_TEST)
size_t curl_write_function(char *ptr, size_t size, size_t nmemb, std::string *userdata)
{
  userdata->append(ptr, size*nmemb);
  return size*nmemb;
}

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

std::string base64_decode(const std::string &in) {

    std::string out;

    std::vector<int> T(256,-1);
    for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i; 

    int val=0, valb=-8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val<<6) + T[c];
        valb += 6;
        if (valb>=0) {
            out.push_back(char((val>>valb)&0xFF));
            valb-=8;
        }
    }
    return out;
}
#endif

bool dasPostToServer(const std::string& url, const std::string& postInfo, std::string& out_response)
{
#if !defined(INTEGRATION_TEST)
  (void) out_response;
  fprintf(stdout, "Posting a body of size %zd to '%s'\n", postInfo.size(), url.c_str());
  return true;
#else
  long site_response_code = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  CURL *curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    std::string postBody("Action=SendMessage&Version=2011-10-01&MessageBody=");
    std::string postBody64 = base64_encode(postInfo);
    postBody += curl_easy_escape(curl, postBody64.c_str(), postBody64.size());
    fprintf(stdout, "Sending: %s\n",  + postBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBody.c_str());

    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    std::string lengthHeader("Content-Length: ");
    lengthHeader += std::to_string((unsigned long)postBody.size());
    headers = curl_slist_append(headers, lengthHeader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    std::string response_info("");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_info);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_function);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stdout, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &site_response_code);
    fprintf(stdout, "Response code: %ld\n", site_response_code);
    out_response.append(response_info);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
  }
  curl_global_cleanup();

  if(site_response_code == 200) return true;
  return false;
#endif
}

#ifdef __cplusplus
}
#endif

