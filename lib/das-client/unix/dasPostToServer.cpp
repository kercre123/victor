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

#ifdef __cplusplus
extern "C" {
#endif

bool dasPostToServer(const std::string& url, const std::string& postBody)
{
  fprintf(stdout, "Posting a body of size %zd to '%s'\n", postBody.size(), url.c_str());
  return true;
}

#ifdef __cplusplus
}
#endif

