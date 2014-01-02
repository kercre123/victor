#include <sipp.h>


//####################################################################################
//TBD: make this inline !
//Record Input and Output line pointers for current filter in the double-buffer
//####################################################################################
static void sippDbRecord(SippFilter *fptr)
{
    UInt32 i;

    //Record in current job the INPUT pointers:
    for(i=0; i<fptr->nParents; i++) //for all parents
       fptr->dbLinesIn[i][fptr->schNo&1] = &((UInt32*)fptr->parents[i]->linePtrs)[fptr->parentsKS[i]];

   //Record in current job the OUTPUT ptr:
    if(fptr->nCons)
      fptr->dbLineOut[fptr->schNo&1] = fptr->outLinePtr;
}

//####################################################################################
// Main Line setup routine
//####################################################################################
static void sippLinePrepare(SippPipeline *pl, int iteration)
{
  SippFilter  *fptr;
  UInt32       i;

  //Record previous schedule decision
    pl->old_mask = pl->can_run_mask;

  //Read schedule decision (for current iteration)
    pl->can_run_mask = pl->schedInfo[iteration].allMask;

  //=============================================================================
  //Output Buffer progress for runnable tasks:
  //i.e. all filters that finised in Previous iteration will get a roll now:
  //=============================================================================
  for (i = 0; i < pl->nFilters; i++)
  {
    fptr = pl->filters[i]; //ptr to i-th filter

    //Roll output buffer only for filters that have output buffers (or consumers)
    //TBD: unify this... somewhere I use nCons, in other places nLines etc...
    //Note: We do buffer rolling based on old_mask, because the data needs to be moved
    //      in consummers space immediately after it was produced, we don't need to wait
    //      until the filter produces new data ! (for a subsampler say after 4 iterations)
    if ((pl->old_mask & (1<<i)) && (fptr->nCons)) //OPT: could use a mask here to optimize
    {
         //alu: get ptr to prev line, move to next one and compute Consumers pointers
         fptr->outLinePtrPrev = fptr->outLinePtr;//this is for CAPTURE feature

         fptr->linePtrs+=4; //stupid pointer stuff... moving through at table of pointers with a U8 ptr.
         if(fptr->linePtrs == fptr->linePtrs3rdBase)
            fptr->linePtrs  = fptr->linePtrs2ndBase;

         //New line that gets into buffer is the new output line for current scheduled iter
         fptr->outLinePtr = (UInt8*)(*(((UInt32*)fptr->linePtrs) + (fptr->nLines-1)));
         //TBD: handle bottom replication
     } //if(can_run)
  }//loop through all filters


  //=============================================================================
  //Once all pointers are advanced, we can sample the pointers and compute line params
  // for this scheduled run; it needs to be done AFTER buffers advancements,
  // otherwise it depends on order in which buffers get updated ?
  //=============================================================================
  for (i = 0; i < pl->nFilters; i++)
  {
    fptr = pl->filters[i]; //ptr to i-th filter

    if (pl->can_run_mask & (1<<i)) 
    {
         //alu: If it didn't finish execution, also enqueue for execution
          if(fptr->schNo < fptr->outputH)
          {
           //Record the I/O pointers
             sippDbRecord(fptr);               

             fptr->lnToPad[fptr->schNo & 1] = fptr->outLinePtrPrev;
             fptr->schNo++;
          }
    }
  }

  //====================================================================
  //for debug randomize the lists per exec unit...
  //====================================================================
  #if defined(SIPP_PC) && 1
   if (pl->dbgLevel > 0) {
     sippDbgDumpRunMask       (pl->can_run_mask, iteration, 0);
   }
  #endif
}

//####################################################################################
// - calls "func_new_frame" for each filter that has one
// - init  "line pointes" for all buffers
// - reset counters.... 
//####################################################################################
static void sippFrameReset(SippPipeline *pl)
{
    SippFilter  *fptr;
    UInt32     i,j;

  #if (defined(SIPP_PC) || defined(MYRIAD2))
  //All HW units start the frame using MAIN set
    pl->shadowSelect = 0x00000000;
    SET_REG_WORD(SIPP_SHADOW_SELECT_ADR, pl->shadowSelect);
  #endif

  //Reset stuff at the beginning of a frame
    for (i=0; i<pl->nFilters; i++) 
    {
        fptr = pl->filters[i];

      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //Reinit line pointers for Top Replication, and fast rolling
        if(fptr->nLines)
        {
            UInt8 **ppp = fptr->linePtrs1stBase;
          //i0:
            for(j=0; j<fptr->nLines-1; j++)
            {
            
                ppp[j] = fptr->outputBuffer          +
                        (0/*idx*/ * fptr->lineStride + fptr->hPadding) * fptr->bpp;
            }
          //i1, i2:
            for(j=0; j<fptr->nLines; j++)
            {
                ppp[j+(fptr->nLines-1)] = fptr->outputBuffer          +
                                     (j * fptr->lineStride + fptr->hPadding) * fptr->bpp;
                ppp[j+(fptr->nLines-1)+fptr->nLines] = ppp[j+(fptr->nLines-1)];
            }
          //i3:
            ppp[3*fptr->nLines-1] = ppp[0];

            fptr->linePtrs        = (UInt8*)&ppp[0];
            fptr->linePtrs2ndBase = (UInt8*)&ppp[fptr->nLines*1];
            fptr->linePtrs3rdBase = (UInt8*)&ppp[fptr->nLines*2];
        }
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

      //Clear counters
        fptr->exeNo = 0;
        fptr->schNo = 0;

      //Filters that map on HW units:
        if(fptr->iBuf[0]) fptr->iBuf[0]->ctx = 0;
        if(fptr->iBuf[1]) fptr->iBuf[1]->ctx = 0;
        if(fptr->oBuf   ) fptr->oBuf->ctx    = 0;
        
        if(fptr->nLines)
        {
           fptr->outLinePtr      = fptr->linePtrs1stBase[0];
           fptr->outLinePtrPrev  = NULL;
           fptr->outputBufferIdx = 0;
        }

      //Build dump file names:
       #ifdef SIPP_F_DUMPS
        sippDbgCreateDumpFiles(pl);
       #endif
    }
}


//###########################################################################
//Starts Shaves, DMAs and SIPP-HW-units that are used Single-Context !
//###########################################################################
//  logica de H-padding: iteratia N+1 poate rula daca toate bufferele de intrare
//  pentru iteratia N sunt finalizate. Asa se va face WAIT-ul.
//  Moment in care, setez H-padding pe aceste filtre ce vor fi folosite?
static void sippStartUnits(SippPipeline *pl)
{
 #if defined(SIPP_PC) || defined(MYRIAD2)
    UInt32 maskSippStart = pl->schedInfo[pl->iteration].sippHwStartMask;
      
  //================================================================
  //1) Quick start of all HW Sipp that don't require special line-based Leon setup
  //   (i.e. all except RAW, POLY) and run in SingleContext mode.
      SET_REG_WORD(SIPP_START_ADR, maskSippStart & (~(SIPP_RAW_ID_MASK|
                                                      SIPP_UPFIRDN_ID_MASK)) );
 #endif

  //================================================================
  //2) Quick start of Shaves
    if(pl->schedInfo[pl->iteration].shaveMask) 
    { 
       #if defined(__sparc) && defined(MYRIAD2)
          pl->svuCmd = 1;
          //Or even better, to remove the Shave test:
          //pl->svuCmd = pl->schedInfo[pl->iteration].shaveMask;
       #else //PC sim and Myriad1
          sippKickShave(pl);
       #endif
    }
 
 #if defined(SIPP_PC) || defined(MYRIAD2)
  //================================================================
  //3) Start of SIPP filters that require Leon setup per line.
  //   We do this step towards the end, as we don't want to keep
  //   the other filters or SHAVES stalled.
  //   After HW-units get the additinal config, they're started via
  //   individual kicks

  //3.1) Start of RAW filter:   
    if(maskSippStart & SIPP_RAW_ID_MASK)
    {
       sippSetupRawLine(pl->hwSippCurFlt[SIPP_RAW_ID], 0);
       SET_REG_WORD(SIPP_START_ADR, SIPP_RAW_ID_MASK);
    }

  //3.2) Rescale needs updated parameters at every iteration
    if(maskSippStart & SIPP_UPFIRDN_ID_MASK)
    {
       sippSetupPolyLine(pl->hwSippCurFlt[SIPP_UPFIRDN_ID], 0);
       SET_REG_WORD(SIPP_START_ADR, SIPP_UPFIRDN_ID_MASK);
    }
 #endif //Sipp HW exists only on Myriad2 or PC sim

  //================================================================
  //4) DMA is very unlikely to be the bottleneck, is likely to finish
  //   very quickly, so start it last
    if(pl->schedInfo[pl->iteration].dmaMask & DMA_MASK)
    {
        sippKickDma(pl);      
    }
}

//###########################################################################
static void sippWaitUnits(SippPipeline *pl)
{
    UInt32     j, status, waitMask;

  #if defined(SIPP_F_DUMPS)
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //Step 0: on PC, all models are blocking, so can do dumps here.
    // no need to wait() for units to execute
    if (pl->dbgLevel > 0) {
      sippDbgDumpFilterOuts(pl);
    }
  #endif

   //Can wait for DMAs or Shave or SIPP-HW
   //  quick units are checked first, so that when slowest units
   //  are done, we just exit.

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //Step 1: Then, can also wait for DMA
   //OPT: Could just remove the test and CALL the wait anyway,
   //     as if not called, will return immediately, else wait.
   //     In general, DMAs are used in each iteration, so can rem the test
    if(pl->schedInfo[pl->iteration].dmaMask & DMA_MASK)
    {
        sippWaitDma(pl->schedInfo[pl->iteration].dmaMask);
    }

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //Step 2: Then, can also wait for Shave
   //OPT: remove the test, just inline the wait for MASTER-SHAVE
    if(pl->schedInfo[pl->iteration].shaveMask)
    {
      #if defined(__sparc)
       #if defined(MYRIAD2)
        //swcLeonReadNoCacheU32 bypasses Leon-L1 data cache
        //| 0x08000000          bypasses Leon-L2 data cache
         while(swcLeonReadNoCacheU32(((UInt32)&pl->svuCmd) | 0x08000000))
         {
           NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
           NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
         }
       #else //Myriad1
         sippWaitShave(pl);
       #endif
      #endif
    }
    
    

  #if defined(SIPP_PC) || defined(MYRIAD2)
   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //Step 3: wait for SIPP-HW Single Context case as the multi-ctx
   //        case is handled differently
   if(pl->multiHwCtx == 0)
   {
      //If barrier is nonzero, then we got to wait for something.
        waitMask = pl->schedInfo[pl->iteration].sippHwWaitMask;
        if(waitMask) 
        {
         //Wait till status bit gets set
           do{
             status = GET_REG_WORD_VAL(SIPP_INT1_STATUS_ADR);
             #if defined(MYRIAD2)
              NOP;NOP;NOP;NOP;NOP;
             #endif
           }while ((status & waitMask) != waitMask); //for DS
   
          //Clear status bits
           SET_REG_WORD(SIPP_INT1_CLEAR_ADR, waitMask);
        }
   }
  #endif

   //=============================================================================
   //Increment ExecNum for all Filters that we waited for, 
   //since they're all done
   //it needs to be done while all are done, else race conditions can occur
   //for example shaves use execNo of HW filters to do padding (to pick the line ptr to be padded)
   //so for now, this update needs to be synchronous.

   for(j=0; j<pl->nFilters; j++)
   {
       if(pl->schedInfo[pl->iteration].allMask & (1<<j))
           pl->filters[j]->exeNo++;
   }
}

void sippOneTimeSetup(SippPipeline *pl); //sippCreate.c

//###########################################################################
void sippHandleCtxUnit(SippPipeline *pl, UInt32 unitID, UInt32 hwStatus)
{
 #if (defined(SIPP_PC) || defined(MYRIAD2))
   UInt32 *ctxNfo = (pl->schedInfoCtx + pl->iteration * pl->schedInfoCtxSz) + pl->hwSippCtxOff[unitID];
   UInt32  curCmd = ctxNfo[pl->hwSippCurCmd[unitID]];
   SippFilter **filters = pl->filters;

   //#if defined(SIPP_PC)
   ///*DBG*/ if(pl->hwSippCurCmd[unitID] == 0)
   ///*DBG*/    DBG_PRINT("[SIPP_START_ADR set(%d) ", (curCmd >> 19) & 1);
   //#endif
   
  //=======================================================
  //If we reach a null comamnd, and HW unit is idle
  // then we're done with current HW-unit in current iteration, so mark the
  // status bit accordingly
  if((curCmd == 0x00000000) && (hwStatus & (1<<unitID)) )
  {
      pl->hwSippCtxSwMask |= (1<<unitID); //mark we're done
      pl->hwSippCurCmd[unitID] = 0;       //reset cmdNo counter for next iter
    //Clear the status bit
      SET_REG_WORD(SIPP_INT1_CLEAR_ADR,  (1<<unitID));
      return;
  }

 //For first command, (CurCmd==0), we don't need to wait on status bit as unit is IDLE
 //    else, subsequent commands can only be executed when module is IDLE
  if(  (pl->hwSippCurCmd[unitID] == 0) ||
      ((pl->hwSippCurCmd[unitID]  > 0) && (hwStatus & (1<<unitID))) )
  {
      UInt32 set   = (curCmd>>19)&0x01;

      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       if((1<<20) & curCmd)
       {
           UInt32 oldID = (curCmd>>13)&0x3F;
           UInt32 newID = (curCmd>> 7)&0x3F;
           
          DBG_PRINT(" [");
          DBG_PRINT("Kick(set=%d,old=%d,new=%d) ",set, oldID, newID);

       //Before writing to I(O)CTX, set the shadowSelect accordingly:
	   //clear old bit, set new value
           pl->shadowSelect = (pl->shadowSelect & ~(1<<unitID)) | (set << unitID);
           SET_REG_WORD(SIPP_SHADOW_SELECT_ADR, pl->shadowSelect);
	   
	   //Optional call of Line Init
       //For most filters this is null (except: RAW, Poly, DS, CC)
           if(pl->hwFnLnIni[unitID])
              pl->hwFnLnIni[unitID](filters[newID], set);

       //CTX switch if new filter is different than old filter
           if(oldID != newID)
           {
               pl->hwCtxSwc[unitID](pl, filters[newID], filters[oldID], unitID);
               //TBD: optimize this, don't use functions
               //so hW_FILT can be reused...
                //saveCtx(pl, filters[oldID], unitID);
                //loadCtx(pl, filters[newID], unitID);
           }
           
       //Start the HW unit: 
           SET_REG_WORD(SIPP_INT1_CLEAR_ADR,  1<<unitID);
           SET_REG_WORD(SIPP_START_ADR,       1<<unitID);
       }
       //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       if((1<<6) & curCmd)
       {
           DBG_PRINT("Load(%d) ", curCmd & 0x1F);
           pl->hwFnLoad[unitID](filters[curCmd & 0x1F], 1-set);
       }
       DBG_PRINT("] ");
      //move to next command
       pl->hwSippCurCmd[unitID]++;
  }
 #endif
}

//###########################################################################
void sippHandleCtxSwitch(SippPipeline *pl)
{
 #if (defined(SIPP_PC) || defined(MYRIAD2))
   UInt32 hwStatus, unitID;
   UInt32 iter = pl->iteration;
   UInt32 multiCtxWaitMask = pl->schedInfoCtx[pl->iteration * pl->schedInfoCtxSz + 0];

 //At start, assume no filter finished
   pl->hwSippCtxSwMask = 0x00000000;

 //Poll & manage all HW units
   do{ 
     //Read the INT status regs to see what filters are IDLE
       hwStatus = GET_REG_WORD_VAL(SIPP_INT1_STATUS_ADR);
   
     //TBD: create a list with all units ever used in this pipeline
     //     so that we do less polling
       for(unitID = SIPP_RAW_ID;  unitID <= SIPP_DBYR_PPM_ID;   unitID++)
       {
         if(multiCtxWaitMask & (1<<unitID))
         {
             sippHandleCtxUnit(pl, unitID, hwStatus);
         }
       }
   }while((pl->hwSippCtxSwMask & multiCtxWaitMask) != multiCtxWaitMask);
 #endif
}

//#############################################################################
void sippWait(UInt32 x)
{
   while(--x)
   {
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
      NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
   }
}

//####################################################################################
void sippProcessFrame(SippPipeline *pl)
{
   //If we're just starting first frame, finish initializations
   //and load the code. The reason for doing mbin load so LATE: we use the slices
   //as temp buffer for scheduler (SippSched).
    if(pl->gi.curFrame == 0)
    {
      sippOneTimeSetup(pl);
    }

    sippFrameReset(pl);
    pl->can_run_mask = 0;
    sippLinePrepare(pl, 0/*iteration*/); //Initial setup

  //The MAIN iteration loop
    for(pl->iteration=0; pl->iteration<pl->nIter-1; pl->iteration++)
    {
        sippStartUnits (pl);                  //Start current iteration
        sippLinePrepare(pl, pl->iteration+1); //Setup for next line in background

        #if (defined(SIPP_PC) || defined(MYRIAD2))
         if(pl->multiHwCtx)
             sippHandleCtxSwitch(pl);
        #endif

        sippWaitUnits  (pl);                  //Wait for units to finish current Iter

       //Print something to indicate progress in VCS
        VCS_PRINT_INT(0xABC00000 + pl->iteration); 
    }

  //Last line:
    sippStartUnits (pl);  //Start current Sch Index
    sippWaitUnits  (pl);  //Wait for units to finish

  #if defined(MYRIAD2)
    sippWait(20); //wait till last DDR writes complete...
  #endif

 //Frame check : only try this on PC
  #if defined(SIPP_PC) && 0
    sipp_frame_check(pl);//Hide this in "sippProcessFrame" if defined SIPP_PC
  #endif

  pl->gi.curFrame++;
}
