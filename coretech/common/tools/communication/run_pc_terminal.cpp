#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

#include "serial.h"
#include "threadSafeQueue.h"

#include "anki\common\robot\utilities.h"

#undef printf
_Check_return_opt_ _CRTIMP int __cdecl printf(_In_z_ _Printf_format_string_ const char * _Format, ...);

#include <queue>

#include <tchar.h>
#include <strsafe.h>

#define PRINTF_BUFFER_SIZE 100000

using namespace std;

// Based off example at http://msdn.microsoft.com/en-us/library/windows/desktop/ms682516(v=vs.85).aspx
DWORD WINAPI PrintfBuffers(LPVOID lpParam)
{
  HANDLE hStdout;
  ThreadSafeQueue<char*> *buffers = (ThreadSafeQueue<char*>*)lpParam;

  TCHAR msgBuf[PRINTF_BUFFER_SIZE];
  size_t cchStringSize;
  DWORD dwChars;

  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

  while(true) {
    if(buffers->IsEmpty()) {
      Sleep(10);
      continue;
    }

    char * nextString = buffers->Pop();

    // Make sure there is a console to receive output results.
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if( hStdout == INVALID_HANDLE_VALUE )
      return 1;

    // Print the parameter values using thread-safe functions.
    StringCchPrintf(msgBuf, PRINTF_BUFFER_SIZE, nextString);
    StringCchLength(msgBuf, PRINTF_BUFFER_SIZE, &cchStringSize);
    WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);

    free(nextString); nextString = NULL;
  } // while(true)

  return 0;
} // DWORD WINAPI PrintfBuffers(LPVOID lpParam)

void printUsage()
{
  printf(
    "usage: terminal <comPort> <baudRate>\n"
    "example: terminal 8 1000000\n");
} // void printUsage()

int main(int argc, char ** argv)
{
  ThreadSafeQueue<char*> buffers = ThreadSafeQueue<char*>();
  Serial serial;

  s32 comPort = 8;
  s32 baudRate = 1000000;

  if(argc == 1) {
    // just use defaults, but print the help anyway
    printUsage();
    printf("using defaults comPort=%d baudRate=%d\n", comPort, baudRate);
  } else if(argc == 3) {
    sscanf(argv[1], "%d", &comPort);
    sscanf(argv[2], "%d", &baudRate);
  } else {
    printUsage();
    return -1;
  }

  if(serial.Open(comPort, baudRate) != RESULT_OK)
    return -1;

  DWORD threadId;
  HANDLE threadHandle = CreateThread(
    NULL,                   // default security attributes
    0,                      // use default stack size
    PrintfBuffers,       // thread function name
    &buffers,          // argument to thread function
    0,                      // use default creation flags
    &threadId);   // returns the thread identifier

  //SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

  const s32 bufferLength = 1024;

  void * buffer = malloc(bufferLength);

  while(true) {
    if(!buffer)
    {
      printf("\n\nCould not allocate buffer\n\n");
      return -4;
      //continue;
    }

    DWORD bytesRead = 0;
    if(serial.Read(buffer, bufferLength-2, bytesRead) != RESULT_OK)
      return -3;

    if(bytesRead > 0) {
      /*     for(u32 i=0; i<bytesRead; i+=4) {
      u32 value = 0;
      value = static_cast<u32>(reinterpret_cast<char*>(buffer)[i+3]) +
      (static_cast<u32>(reinterpret_cast<char*>(buffer)[i+2])<<8) +
      (static_cast<u32>(reinterpret_cast<char*>(buffer)[i+1])<<16) +
      (static_cast<u32>(reinterpret_cast<char*>(buffer)[i])<<24);

      printf("%d ", static_cast<s32>(value));
      }

      printf("\n");
      */
      reinterpret_cast<char*>(buffer)[bytesRead] = '\0';

      buffers.Push(reinterpret_cast<char*>(buffer));

      buffer = malloc(bufferLength);
    }
  }

  if(serial.Close() != RESULT_OK)
    return -2;

  return 0;
}
#endif