#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

#include "serial.h"
#include "anki\common\robot\utilities.h"

#undef printf
_Check_return_opt_ _CRTIMP int __cdecl printf(_In_z_ _Printf_format_string_ const char * _Format, ...);

#include <queue>

#include <tchar.h>
#include <strsafe.h>

#define PRINTF_BUFFER_SIZE 100000

using namespace std;

class ThreadSafeStringBuffer
{
public:
  ThreadSafeStringBuffer()
  {
    mutex = CreateMutex(NULL, FALSE, NULL);
  } // ThreadSafeStringBuffer()

  string Pop()
  {
    string value;

    WaitForSingleObject(mutex, INFINITE);

    if(buffers.empty()) {
      value = "";
    } else {
      value = buffers.front();
      buffers.pop();
    }

    ReleaseMutex(mutex);

    return value;
  } // string Pop()

  void Push(string newString)
  {
    WaitForSingleObject(mutex, INFINITE);

    buffers.push(newString);

    ReleaseMutex(mutex);
  } // void Push(string newString)

  bool IsEmpty()
  {
    bool isEmpty;

    WaitForSingleObject(mutex, INFINITE);

    if(buffers.empty()) {
      isEmpty = true;
    } else {
      isEmpty = false;
    }

    ReleaseMutex(mutex);

    return isEmpty;
  }
protected:
  HANDLE mutex;

  queue<string> buffers;
}; // class ThreadSafeStringBuffer

// Based off example at http://msdn.microsoft.com/en-us/library/windows/desktop/ms682516(v=vs.85).aspx
DWORD WINAPI PrintfBuffers(LPVOID lpParam)
{
  HANDLE hStdout;
  ThreadSafeStringBuffer *buffers = (ThreadSafeStringBuffer*)lpParam;

  TCHAR msgBuf[PRINTF_BUFFER_SIZE];
  size_t cchStringSize;
  DWORD dwChars;

  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

  while(true) {
    if(buffers->IsEmpty()) {
      Sleep(10);
    }

    const string nextString = buffers->Pop();

    // Make sure there is a console to receive output results.
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if( hStdout == INVALID_HANDLE_VALUE )
      return 1;

    // Print the parameter values using thread-safe functions.
    StringCchPrintf(msgBuf, PRINTF_BUFFER_SIZE, nextString.data());
    StringCchLength(msgBuf, PRINTF_BUFFER_SIZE, &cchStringSize);
    WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
  } // while(true)

  return 0;
} // DWORD WINAPI PrintfBuffers(LPVOID lpParam)

int main(int argc, char ** argv)
{
  ThreadSafeStringBuffer buffers;
  Serial serial;

  buffers = ThreadSafeStringBuffer();

  if(serial.Open(8, 1500000) != RESULT_OK)
    return -1;

  DWORD threadId;
  HANDLE threadHandle = CreateThread(
    NULL,                   // default security attributes
    0,                      // use default stack size
    PrintfBuffers,       // thread function name
    &buffers,          // argument to thread function
    0,                      // use default creation flags
    &threadId);   // returns the thread identifier

  const s32 bufferLength = 10000;
  void * buffer = malloc(bufferLength);

  while(true) {
    DWORD bytesRead = 0;
    if(serial.Read(buffer, bufferLength-1, bytesRead) != RESULT_OK)
      return -3;

    if(bytesRead > 0) {
      reinterpret_cast<char*>(buffer)[bytesRead+1] = '\0';

      buffers.Push(reinterpret_cast<char*>(buffer));
    }
  }

  if(serial.Close() != RESULT_OK)
    return -2;

  return 0;
}
#endif