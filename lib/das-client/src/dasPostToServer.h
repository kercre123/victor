/**
 * File: dasPostToServer.h
 *
 * Author: seichert
 * Created: 07/16/2014
 *
 * Description: Function to be implemented on a per platform basis
 *              to do an HTTP POST to a given URL with the supplied
 *              postBody
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __DasPostToServer_H__
#define __DasPostToServer_H__

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

bool dasPostToServer(const std::string& url, const std::string& postBody, std::string& out_response);

#ifdef __cplusplus
}
#endif

#endif // __DasPostToServer_H__
