/**
 * File: dasDemoApp
 *
 * Author: seichert
 * Created: 01/15/2015
 *
 * Description: Write out DAS logs to stdout
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "DAS.h"
#include "portableTypes.h"
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <string>

int main(int argc, char *argv[]) {
  std::string gameLogDir = "./gameLogs";

  printf("Simple DAS Demo App\n");
  (void) mkdir(gameLogDir.c_str(), 0777);

  uuid_t gameUUID;
  uuid_generate(gameUUID);
  uuid_string_t gameID;
  uuid_unparse_upper(gameUUID, gameID);


  DASConfigure("DASConfig.json", "./dasLogs", gameLogDir.c_str());
  DAS_SetGlobal("$game", gameID);
  DAS_SetGlobal("$phys", "0xbeef1234");
  DAS_SetGlobal("$app", "version 1.3,b");
  DASEvent("TestEvent.Main", "Hello \"World\"");
  DASEvent("TestEvent.Start", "This is line #1.\nThis is line #2.\nAnd number three");
  DAS_SetGlobal("$game", nullptr);
  sleep(10);
  DASClose();
  return 0;
}
