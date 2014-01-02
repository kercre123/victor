#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//####################################################################################
//Macros to manage the switch commands (32bit/command)
//
#define CMD_KICK(set, oldID, newID) ((1<<20)|(set<<19)|(oldID<<13)|(newID<<7))
#define CMD_LOAD(id)                ((1<< 6)|( id    ))


//####################################################################################
//Figure out if we need HW multi-context support
//The following filters have NO shadow registers:
//  -  Colour combination
//  -  MIPI Rx filters
//  -  MIPI Tx filters
void sippGetCtxOrder(SippPipeline *pl)
{
    UInt32 unitID;

  //Assume no multi-ctx support is needed
    pl->multiHwCtx = 0;

  //If at least one of these units run at least 2 filters (filterCount > 1)
  //then we need multi-ctx routines
    for(unitID=SIPP_RAW_ID; unitID<=SIPP_DBYR_PPM_ID; unitID++)
    {
        if(pl->hwSippFltCnt[unitID] > 1)
        {
            pl->multiHwCtx = 1; 
            return;
        }
    }
}

//####################################################################################
//Check in the filter list and see if a filter is to run on the "unitID" in iteration "iter"
int unitUsedInIter(SippPipeline *pl, UInt32 iter, UInt32 unitID)
{
    int i;

    for(i=0; i<pl->nFilters; i++)
    {
        if((pl->schedInfo[iter].allMask & (1<<i)) &&
            (pl->filters[i]->unit == unitID) )
        {
            return 1;
        }
    }

    return 0;
}

//####################################################################################
//Returns bool value indicating if filter with ID==fldID will run on
// hw-unit given by "unitID" in the immediately NEXT iteration where "unitID"
// hw-unit is invoked. The next-iteration doesn't have to be iter+1 necessarily
// ;can be anything from >curIter till the end of sim
int runsInNextIter(SippPipeline *pl, UInt32 fltID, UInt32 curIter, UInt32 unitID)
{
    int i;
    int runs = 0; //false, just in case HW unit is never used from curIter+1 onwards

    //If current filter didn't finish yet,
    if(pl->filters[fltID]->exeNo < pl->filters[fltID]->outputH)
    {
      //Check future iterations
      for(i=curIter+1; i<pl->nIter; i++)
      {
         //The first future iteration where HW unit is used,
         //gives the answer: just check if our filter is USED or NOT in that iteration
         if(unitUsedInIter(pl, i, unitID))
         {
             //And filter is set to run, then return 1
             if(pl->schedInfo[i].allMask & (1<<fltID) ) runs = 1;
             else                                       runs = 0;

             break;
         }
      }
    }

    return runs;
}


//####################################################################################
// Returns the ID of the first SW-filter in a future iteration that runs on HW unit
// given by "unitID"
int getFutureIterFid(SippPipeline *pl, UInt32 curIter, UInt32 unitID)
{
    UInt32 i, id;

  //Look in fugure iterations: see if HW unit is used
    for(i=curIter+1; i<pl->nIter; i++)
    {
       //If HW unit is used in current iteration,
       //then we got our answer:
       //if(pl->schedInfo[i].sippHwStartMask & (1<<unitID))
       if(unitUsedInIter(pl, i, unitID))
       {
           //Get first filter in bit-scan order that is to execute
           for(id = 0; id<pl->nFilters; id++)
           {
             if( (pl->filters[id]->unit == unitID) &&
                 (pl->filters[id]->exeNo < pl->filters[id]->outputH) &&
                 (pl->schedInfo[i].allMask & (1<<id))  )
             {
                 return id;//filter number
             }
           }
       }
    }

  //Else, no filters are to run anymore in future on this HW unit
   return -1;
}



//####################################################################################
//                 _              _                   _              _ 
//                (_)            | |                 | |            | |
// _ __ ___   __ _ _ _ __     ___| |___  __  ___  ___| |__   ___  __| |
//| '_ ` _ \ / _` | | '_ \   / __| __\ \/ / / __|/ __| '_ \ / _ \/ _` |
//| | | | | | (_| | | | | | | (__| |_ >  <  \__ \ (__| | | |  __/ (_| |
//|_| |_| |_|\__,_|_|_| |_|  \___|\__/_/\_\ |___/\___|_| |_|\___|\__,_|
//                                                                    
//computes the schedule for a HW unit given by "unitID"
//Each context switch command is 32bit... see description above
//####################################################################################

void sippUpdateUnitSippCtx(SippPipeline *pl, UInt32 unitID)
{ 
    Int32 i, k, n, cmdNo, nxt, found;
    Int32 lastKickID; //index of last filter in a Previously executed iteration

    SippFilter *unitLst[8];
    SippFilter *aux;
    UInt32     *cmdArr;
    UInt32      newID;

  //Double ctx: Main and Shadow for current filter
  //            assume initially that they're both free
  //These are used to figure out which context to KICK first within current iter
    UInt8       ctxNo = 0; //0:main 1:shadow
    
  //At start/initialization, first filter gets loaded on Main set
    lastKickID    = pl->hwSippFirst[unitID];

  //Initial filter was already loaded, so increment it's load count (opt for 2-filts / HW instance case)
    pl->filters[lastKickID]->nCtxLoads++;

  //For all Iterations
    for(i=0; i<pl->nIter; i++)
    {
        //The multi-ctx Unit start_wait mask 
        pl->schedInfoCtx[i * pl->schedInfoCtxSz] = 0;

       //If HW-unit is multi-ctx, AND is used in current iteration
        //if(pl->schedInfo[i].sippHwStartMask & (1<<unitID))
        if(unitUsedInIter(pl, i, unitID))
        {
          //Get reference to the CTX-command array allocated for this HW-unit, as we'll need
          //to populate it with ctx-switch commands
           cmdArr = pl->schedInfoCtx + (i * pl->schedInfoCtxSz) + pl->hwSippCtxOff[unitID];

           pl->schedInfoCtx[i * pl->schedInfoCtxSz] |= (1<<unitID);

          //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          //1) Build a list with all filters that are to run in current iteration
          //   on current HW_unit
            n = 0;
            for(k=0; k<pl->nFilters; k++)
            {
                if( (pl->filters[k]->unit == unitID)      &&
                    (pl->schedInfo[i].allMask & (1<<k))   &&
                    (pl->filters[k]->exeNo < pl->filters[k]->outputH) )
                  {
                    pl->filters[k]->exeNo++;    //will run, so increment execNo
                    unitLst[n] = pl->filters[k];//and add to filter to list
                    n++;
                  }
            }
 
           #if 0
            DBG_PRINT("("); for(k=0; k<n; k++) DBG_PRINT(" %d, ", unitLst[k]->id); DBG_PRINT(") ");
           #endif

          //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          //2)Reorder so that First filter in the list is the same as PREV from previous iteration
          //  Search where lastInPtrIter is in the local list
              found = 0;
              for(k=0; k<n; k++)
              {
                  if(unitLst[k] == pl->filters[lastKickID])
                  {
                      found = 1;
                      break;
                  }
              }

            //Swap entries : K with 0 if possible
              if(found)
              {
                  aux        = unitLst[k];
                  unitLst[k] = unitLst[0];
                  unitLst[0] = aux;
              }

          //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          //3)Generate the ctx-switch commands
            DBG_PRINT(" \n ITER: %4d :", i);
            k = 0;
            for(cmdNo=0; cmdNo<n; cmdNo++)
            {
                cmdArr[cmdNo] = 0; //clear current command initially

              //######################
              //Kick
              //######################
                if(cmdNo == 0){
                    //filter was previously started with SIPP_START_ADDR
                    //only print a debug message
                    DBG_PRINT(" [SSA(set=%d,old=%d,new=%d) ",  ctxNo, lastKickID, unitLst[k]->id);
                }
                else
                {
                    DBG_PRINT(" [Kick(set=%d,old=%d,new=%d) ", ctxNo, lastKickID, unitLst[k]->id);
                }
                cmdArr[cmdNo] |= CMD_KICK(ctxNo, lastKickID, unitLst[k]->id);
                lastKickID = unitLst[k]->id;

              //######################
              //Save
              //Load
              //######################
                newID = -1; //Figure out the new filter
                if(cmdNo<n-1)
                {
                    newID = unitLst[k+1]->id;
                }
                else{//last run on this unit, prepare for NEXT iteration:
                    if(runsInNextIter(pl, unitLst[k]->id, i, unitID))
                    {
                        //if same filter will run in next iter as well, no CTX switch is to be done
                        newID = lastKickID;
                    }
                    else{ //last filter in current Iter won't run in next iteration when HW is used
                          //so try load the first in next iteration (if exists). If it doesnt' exist, 
                          //don't do anything
                        newID = getFutureIterFid(pl, i, unitID);
                    }
                }

                //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                //If we got something, generate the Save,Load commands
                if((newID != lastKickID) && (newID != -1))
                {
                  //The load goes in the alternate set:
                   ctxNo = 1 - ctxNo;

                   //Optimization test: if only 2 filters are used per instance, then only allow
                   // the first load. If more than 2, then do the usual
                   if(     (pl->hwSippFltCnt[unitID] >2) 
                       || ((pl->hwSippFltCnt[unitID]==2) && (pl->filters[newID]->nCtxLoads == 0)) )
                   {
                      DBG_PRINT("Load(set=%d,id=%2d) ",ctxNo, newID);
                      cmdArr[cmdNo]  |= CMD_LOAD(newID);
                      pl->filters[newID]->nCtxLoads++;
                   }
                }

               //DBG_PRINT("CMD=0x%02x ", cmdArr[cmdNo]);
                DBG_PRINT("]");
                k++;
            }
 
       //Command list terminator
         cmdArr[cmdNo] = 0;
      }
    }

    DBG_PRINT("\n");
}

//###########################################################################
//Generate Load/Kick multi-context commands for Runtime
void sippComputeHwCtxChg(SippPipeline *pl)
{
   UInt32 uID, i;

   if(pl->multiHwCtx == 0)
      return;

  //Clear run-numbers for all filters as we'll use exe numbers
  //to track if filters will really run or not in certain iterations
  //(instead of just rolling the buffs)
  //Each "sippUpdateUnitSippCtx" will update just some filters
  //so we do a common clear here
    for(i=0; i<pl->nFilters; i++)
    {
        pl->filters[i]->exeNo     = 0;
        pl->filters[i]->nCtxLoads = 0; //Important
    }
 
  //Generate ctx-switch commands only for HW-units that
  //need to run at least 2 SW filters
    for(uID= SIPP_RAW_ID; uID<= SIPP_DBYR_PPM_ID; uID++)
     if(pl->hwSippFltCnt[uID] > 1) 
       sippUpdateUnitSippCtx(pl, uID);

  //Once schedule is computed, clear the bits for the units that require

 //Note: sippFrameReset will set exeNo back to 0
}

#endif