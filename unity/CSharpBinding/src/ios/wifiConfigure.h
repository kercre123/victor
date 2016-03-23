/**
 * File: wifiConfigure.h
 *
 * Author: Lee Crippen
 * Created: 03/21/16
 *
 * Description: Handles the ios specific http server functionality for wifi configuration.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __iOS_WifiConfigure_H__
#define __iOS_WifiConfigure_H__


int COZHttpServerInit(int portNum);
  
int COZHttpServerShutdown();
  
bool COZWifiConfigure(const char* wifiSSID, const char* wifiPasskey);


#endif // __iOS_WifiConfigure_H__
