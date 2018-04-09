/**
 * File: main.cpp
 *
 * Author: paluri
 * Created: 2/15/2018
 *
 * Description: main entry-point ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DEBUG DEFINE FOR TESTING ON iOS !!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define IOS_MAIN 1
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

# if !IOS_MAIN
#include "switchboard.h"

int main() {
  Anki::SwitchboardDaemon::Start();
}
# endif

