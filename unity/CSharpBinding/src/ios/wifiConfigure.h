#ifndef __iOS_WifiConfigure_H__
#define __iOS_WifiConfigure_H__


int COZHttpServerInit(int portNum);
  
int COZHttpServerShutdown();
  
bool COZWifiConfigure(const char* wifiSSID, const char* wifiPasskey);


#endif // __iOS_WifiConfigure_H__
