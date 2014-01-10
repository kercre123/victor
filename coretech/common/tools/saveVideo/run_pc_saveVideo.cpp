#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

#include "serial.h"
#include "anki\common\robot\utilities.h"

#undef printf
_Check_return_opt_ _CRTIMP int __cdecl printf(_In_z_ _Printf_format_string_ const char * _Format, ...);

int main(int argc, char ** argv)
{
  Serial serial;

  if(serial.Open(8, 1500000) != RESULT_OK)
    return -1;

  const s32 bufferLength = 10000;
  void * buffer = malloc(bufferLength);

  while(true) {
    DWORD bytesRead = 0;
    if(serial.Read(buffer, bufferLength, bytesRead) != RESULT_OK)
      return -3;

    DWORD i = 0;

    //for(i=0; i<bytesRead; i++) {
    //  printf("%c", reinterpret_cast<char*>(buffer)[i]);
    //}

    if(bytesRead >= 4) {
      for(i=0; i<(bytesRead-3); i+=4) {
        printf("%c%c%c%c",
          reinterpret_cast<char*>(buffer)[i], reinterpret_cast<char*>(buffer)[i+1],
          reinterpret_cast<char*>(buffer)[i+2], reinterpret_cast<char*>(buffer)[i+3]);
      }
    }

    for(; i<bytesRead; i++) {
    //for(i=bytesRead-50; i<bytesRead; i++) {
      printf("%c", reinterpret_cast<char*>(buffer)[i]);
    }
  }

  if(serial.Close() != RESULT_OK)
    return -2;

  return 0;
}
#endif