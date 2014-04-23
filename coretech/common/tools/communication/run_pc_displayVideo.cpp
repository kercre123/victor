#ifndef ROBOT_HARDWARE

#define COZMO_ROBOT

#include "communication.h"
#include "threadSafeQueue.h"
#include "debugStreamClient.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"
#include "anki/common/robot/errorHandling.h"

#ifdef _MSC_VER
#include <tchar.h>
#include <strsafe.h>

#undef printf
_Check_return_opt_ _CRTIMP int __cdecl printf(_In_z_ _Printf_format_string_ const char * _Format, ...);

#else // #ifdef _MSC_VER

#include <unistd.h>
#include <pthread.h>

#endif // #ifdef _MSC_VER ... #else

#include <queue>

#include <ctime>

#include "opencv/cv.h"

using namespace std;
using namespace Anki;
using namespace Anki::Embedded;

static void printUsage()
{
} // void printUsage()

int main(int argc, char ** argv)
{
  // TCP
  const char * ipAddress = "192.168.3.30";
  const s32 port = 5551;

  printf("Starting display\n");

  SetLogSilence(true);

  DebugStreamClient parserThread(ipAddress, port);

  while(true) {
    DebugStreamClient::Object newObject = parserThread.GetNextObject();
    printf("Received %s %s\n", newObject.typeName, newObject.objectName);

    if(newObject.buffer) {
      free(newObject.buffer);
      newObject.buffer = NULL;
    }
  }

  return 0;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
