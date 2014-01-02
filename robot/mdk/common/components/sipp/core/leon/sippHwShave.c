#include <sipp.h>

//Shave symbols used by Master Shave Unit kick
#if defined(SIPP_PC)
extern UInt32 dbg_svu_no;
#endif

extern int               SVU_SYM(SHAVE_MAIN)(void);

//####################################################################################
// [Master Shave unit] kick routine (will kick all shaves in group)
//####################################################################################
void sippKickShave(SippPipeline *pl)
{
   UInt32 s;
   UInt32 sliceFirst = pl->gi.sliceFirst;
   UInt32 sliceLast  = pl->gi.sliceLast;

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   #if defined(SIPP_PC)
      //note: "sipp_pl" is properly initialized in "sipp_init_svus"
      //1)~~~~~~~~~~ Hpadding for all slices:
       pl->svuCmd = CMD_H_PAD | CMD_EXIT;
       for(s=sliceFirst; s<=sliceLast; s++)
       {
           dbg_svu_no = s;
           SHAVE_MAIN();
       }

       //2)~~~~~~~~~~~ Run for all slices
       pl->svuCmd = CMD_RUN | CMD_EXIT;
       for(s=sliceFirst; s<=sliceLast; s++)
       {
           dbg_svu_no = s;
           SHAVE_MAIN();
       }

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   #elif defined(__sparc)
      //unit->gi->pl->svuCmd = CMD_H_PAD | CMD_RUN | CMD_EXIT; //FIX THIS !!!!
      pl->svuCmd = CMD_H_PAD | CMD_RUN | CMD_EXIT; //?????
      for(s=sliceFirst; s<=sliceLast; s++)
      {
          swcResetShave(s); //in Main, compiler generated code initializes STACK
          swcStartShave(s, SVU_SYM(SHAVE_MAIN));
      }
   #endif
}

//####################################################################################
void sippWaitShave(SippPipeline *pl)
{
  #if defined(__sparc)
    UInt32 s;
    UInt32 sliceFirst = pl->gi.sliceFirst;
    UInt32 sliceLast  = pl->gi.sliceLast;

    for(s=sliceFirst; s<=sliceLast; s++)
        swcWaitShave(s);
  #endif
}
