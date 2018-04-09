#include <stdio.h>
#include "board.h" //hardware.h
#include "cmd.h"
#include "contacts.h"
#include "portable.h"
#include "timer.h"

#define CONSOLE_ECHO 0

//------------------------------------------------  
//    manage...stuff
//------------------------------------------------

void contacts_manage(void)
{
  char* line = Contacts::getline(); //polls uart
  if( line && cmd_process(line) < 0 ) { //valid line, not a command
    //if( !CONSOLE_ECHO )
    //Contacts::printf("%s\n", line); //echo random lines?
  }
}

void test_manage_(void)
{
  /*/asynchronously spit some stuff out charge contacts so we can see if its working
  char b[10]; int bz = sizeof(b);
  static uint32_t Ttx=0, txcnt=0;
  if( Timer::elapsedUs(Ttx) >= 10*1000*1000 ) {
    Ttx = Timer::get();
    Contacts::write( snformat(b,bz,"%is\n",txcnt+=10) );
  } //-*/
  
  /*/blinky the led
  static uint32_t Tled=0;
  if( Timer::elapsedUs(Tled) >= 500*1000 ) {
    Tled = Timer::get();
    if( DBGLED::getMode() == MODE_INPUT )
      DBGLED::init(MODE_OUTPUT, PULL_NONE);
    DBGLED::write( !(DBGLED::read()>0) );
  } //-*/
}

//------------------------------------------------  
//    main
//------------------------------------------------

int main(void)
{
  Board::init();
  Timer::init();
  
  Contacts::init(); //charge contact uart
  if( CONSOLE_ECHO ) {
    Contacts::write("\n\n*********** systest ****************\n" __DATE__ __TIME__ "\n");
    Contacts::write("build compatibility: HW " HWVERS "\n");
    Contacts::echo(true);
  }
  
	while(1)
	{
    contacts_manage();
    //test_manage_();
	}
}

