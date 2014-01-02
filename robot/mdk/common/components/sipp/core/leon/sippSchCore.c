#include <sipp.h>

//######################################################################################
//Consumer->parents contains a list of parent pointers (->parents)
//This function returns the INDEX in parents list that corresponds to "parent" argument
//######################################################################################
static UInt32 getParentIndex(SippFilter *consumer, SippFilter *parent)
{
    UInt32 x, found = 0;

    for(x=0; x < consumer->nParents; x++)
    {
      if(consumer->parents[x] == parent)
      {
        found = 1;
        break;
      }
    }

    //If parent is not found, then pipeline is WRONGly build!
    if(!found)
      sippError(E_PAR_NOT_FOUND);
    
    return x;
}

//######################################################################################
//  Consumer (fptrC) asks a list of line indices (reqInLns) from in parent (fptrP)
//  If the [Kernel Start] is not already computed for given parent, then will do a full
//  search else, will only check data at [Kernel Start] position
//######################################################################################
static int lookFor(SippFilter  *fptrP,  
                    SippFilter *fptrC ) 
{
   int    x     = getParentIndex(fptrC, fptrP);
   int    *what = fptrC->sch->reqInLns  [x];
   int    len   = fptrC->nLinesUsed[x];

   int    start, j, found;

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   //If kernel func_start is not set yet, then do a full search
   if((fptrC->parentsKS[x] == (UInt32)-1) || (fptrC->flags & SIPP_RESIZE))
   {
       for(start = fptrP->nLines - len; start>=0;  start--)
       {
         found = 1;
         for(j=0;j<len; j++)
         {
             if(fptrP->sch->lineIndices[start+j] != what[j])
             {
                 found = 0;
                 break;
             }
         }
         if(found == 1)
         {
           return start;
         }
       }
   }
   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   else{//else, only search from KS
       found = 1;
       for(j=0;j<len; j++)
       {
         if(fptrP->sch->lineIndices[fptrC->parentsKS[x]+j] != what[j])
         {
           found = 0;
           break;
         }
       }
       if(found == 1)
         return fptrC->parentsKS[x];
   }

   //If we ever get here, then it's not found!
   return -1;
}



//#####################################################################################
// All sim starts with "BUFF_HUGE_SZ" assumption.
// After everything is done, we can adjust the size of filters by looking at min-KS value
//#####################################################################################
static void minimizeLineBuffs(SippPipeline *pl)
{
    SippFilter *fptr;
    UInt32 i, c, x, min_ks, ks;
    UInt32 nFilters = pl->nFilters;

    for(i=0; i<nFilters; i++)
    {
        fptr = pl->filters[i]; //current filter

      //Find min KS index in all consumers:
        if(fptr->nCons)
        {
            //assume first consumer has min KS
            x  = getParentIndex(fptr->cons[0], fptr);
            min_ks = fptr->cons[0]->sch->parentsKsMin[x];

            //then look in remaining consumers
            for(c=1; c<fptr->nCons; c++)
            {
                x  = getParentIndex(fptr->cons[c], fptr);
                ks = fptr->cons[c]->sch->parentsKsMin[x];
                if(ks < min_ks)
                   min_ks = ks;
            }

          //Compute optimal buffer size by subtracting the smallest
          //KS ever recorded:
            fptr->nLines = fptr->nLines - min_ks;

          //Adjust kernels starts for consumers:
            for(c=0; c<fptr->nCons; c++)
            {
                x  = getParentIndex(fptr->cons[c], fptr);
                fptr->cons[c]->parentsKS[x] -= min_ks;
            }
        }
    }
}

//#####################################################################################
// When a fitler is set to run for it's first time, record the sampling positions
// (i.e. Kernel STarts) in parent buffers !
// These will be later used to sample the parent buffers
//#####################################################################################
static void recordKS(SippFilter *fptr)
{
    UInt32 p;

    //go through all parents:
    for(p=0; p<fptr->nParents; p++)
    {
        fptr->parentsKS[p] = lookFor(fptr->parents[p], fptr);

        if(fptr->parentsKS[p] == (UInt32)-1)
           sippError(E_DATA_NOT_FOUND);// this should never happen: filter is said 
                                       // to have data, yet it's not found

        if(fptr->parentsKS[p] < fptr->sch->parentsKsMin[p])
           fptr->sch->parentsKsMin[p] = fptr->parentsKS[p];
    }
}

//#####################################################################################
static void progBuffer(SippFilter *fptr)
{
    UInt32 i;

    //for accidental cals this for a filter witnout consumer, just return
    if(fptr->nCons == 0)
        return;

    if(fptr->sch->curLine == 0)
    {//Top replication
      for(i=0; i<fptr->nLines; i++)
        fptr->sch->lineIndices[i] = 0;//fptr->sch->curLine;
    }
    else{
      //shift N-1 line; if buffer is just one line, this won't get called
       for(i=0; i<fptr->nLines-1; i++)
        fptr->sch->lineIndices[i] = fptr->sch->lineIndices[i+1];

      //and populate the new line
       if(fptr->sch->curLine < (int)fptr->outputH)
         fptr->sch->lineIndices[i] = fptr->sch->curLine; //next line
       else{//bottom replication 
         #if 0
         //WRONG: this assumes there are AT LEAST 2 lines in buffer, and it's not always the case
          fptr->sch->lineIndices[i] = fptr->sch->lineIndices[i-1]; 
         #else
          fptr->sch->lineIndices[i] = fptr->outputH - 1; //-1 as we start counting from 0
         #endif
       }
    }
}

//#####################################################################################
static RunStatus can_run_P_cond(SippFilter *fptr)
{
    UInt32 p;

    //if current filter finished the data, the PARENT restriction
    //dissapears, so can say "can run"
    if(fptr->sch->curLine >= (int)fptr->outputH-1)
    {
        fptr->sch->dbgJustRoll = 1;
        return RS_CAN_RUN;
    }
    else
        fptr->sch->dbgJustRoll = 0;


    if(fptr->nParents)
    {   
      //Filter states what lines it requires in current run
        fptr->funcAsk(fptr, fptr->sch->curLine+1);
      
      //Then we look through all parents and see if it's available
        for(p=0; p<fptr->nParents; p++)
        {
          if(lookFor(fptr->parents[p], fptr) == -1)
          {//if data is missing, then we can't run this iteration
             return RS_CANNOT;
          }
        }

        //If we got here, it means we can run due to parent conditions
        //If this is the first line to be generated, record KS
        //as for the first line, consumers can never block a filter.
        //if(fptr->curLine == -1)

        //If we ended up here with KS fixed decision, we can safely re-run this.
        //If one linke was reseted (e.g. -1), then we NEED to record the answer now
        //for buffer allocation. So run anyway:
        recordKS(fptr);
        
        return RS_CAN_RUN;
    }
    else{ //if it has no parents, there's nobody stopping 
          //this filter
        return RS_CAN_RUN;
    }
}

//#####################################################################################
static void simInit(SippPipeline *pl)
{
   SippFilter *fptr;
   UInt32    p, i, j;
   UInt32    nFilters = pl->nFilters;

   //number of iterations generated / frame
   pl->nIter = 0;

   //Initializations
   for(i=0; i<nFilters; i++)
   {
       fptr = pl->filters[i];

     //No line index info
      for(j=0; j<fptr->nLines; j++)
        fptr->sch->lineIndices[j] = -1; //empty

     //No info on Kernel Start
      for(p=0; p<fptr->nParents; p++)
      {
          fptr->parentsKS         [p] = -1;
          fptr->sch->parentsKsMin[p] =  10000000;
          fptr->sch->parentsKsClr[p] = 0;
      }

      fptr->sch->curLine = -1;
      fptr->sch->canRunP = RS_DONT_KNOW;
      fptr->sch->canRunC = RS_DONT_KNOW;
   }
}

//#####################################################################################
//                 _                  _              _       _      
//                (_)                | |            | |     | |     
// _ __ ___   __ _ _ _ __    ___  ___| |__   ___  __| |_   _| | ___ 
//| '_ ` _ \ / _` | | '_ \  / __|/ __| '_ \ / _ \/ _` | | | | |/ _ \
//| | | | | | (_| | | | | | \__ \ (__| | | |  __/ (_| | |_| | |  __/
//|_| |_| |_|\__,_|_|_| |_| |___/\___|_| |_|\___|\__,_|\__,_|_|\___|
//                                                                  
//#####################################################################################
static void simSchedule(SippPipeline *pl)
{
   SippFilter **filters  = pl->filters;
   UInt32       nFilters = pl->nFilters;
   SippFilter  *fptr, *cons;
   UInt32       i, p, x, c;
   int          somethingRuns, allKnown;
   int          iteration = 0;

   simInit(pl);

   //======================================================================
   //Main loop
   //======================================================================
   do{
      somethingRuns = 0;

      for(i=0; i<nFilters; i++)
          filters[i]->sch->canRunC = RS_DONT_KNOW;

      //=====================================================================
      //Delayed KS clearing
      //=====================================================================
      for(i=0; i<nFilters; i++)
      {
        for(p=0; p<filters[i]->nParents; p++)
        {
            if(filters[i]->sch->parentsKsClr[p])
            {
                filters[i]->parentsKS        [p] = -1;
                filters[i]->sch->parentsKsClr[p] =  0;//clear flag
            }
        }
      }


    //=====================================================================
    //Decide what filters can run in current iteration
    // P-condition: look in all parents buffers and see if data is available
    //=====================================================================
       for(i=0; i<nFilters; i++)
       {
           filters[i]->sch->canRunP    = can_run_P_cond(filters[i]);
           if(filters[i]->sch->canRunP == RS_CANNOT)
              filters[i]->sch->canRunC =  RS_CANNOT;
       }

    //=====================================================================
    //C-condition: look through all consumers and see if anybody needs
    //             data that's not already in the buffer (in next iteration)
    //=====================================================================
      do{
       allKnown = 1;

       for(i=0; i<nFilters; i++)
       {
           fptr = filters[i];

           if(fptr->sch->canRunP == RS_CAN_RUN)
           {
             if(fptr->nCons == 0){   //if filter has no consumers, then
                fptr->sch->canRunC = RS_CAN_RUN;//C-condition becomes true
             }
             else{

             //Check the impact of running current filter from Consumers 
             //point of view. Do a Save, Trial-advance, Check, Restore cycle 
             int T_indices[BUFF_HUGE_SZ]; 
             memcpy(T_indices, fptr->sch->lineIndices, sizeof(fptr->sch->lineIndices));

             fptr->sch->canRunC = RS_CAN_RUN; //assume we can run...

             //Trial buffer advancement
             fptr->sch->curLine++; 
             progBuffer(fptr);

             for(c=0; c<fptr->nCons; c++)
             { 
                cons = fptr->cons[c];
                x    = getParentIndex(cons, fptr);


               //for all tracking consumers
                if( (cons->sch->curLine+1 < (int)cons->outputH) 
                    && (cons->parentsKS[x] != -1)        )
                {
                  if(cons->sch->canRunC == RS_DONT_KNOW)
                  {
                    fptr->sch->canRunC = RS_DONT_KNOW;
                    allKnown = 0;
                    break;
                  }

                  else{
                    int frst_ln_p, last_ln_p;
                    int frst_ln_c, last_ln_c;

                    //Find consumer expectations for it's run in NEXT ITERATION
                    if(cons->sch->canRunC == RS_CAN_RUN)
                       cons->funcAsk(cons, cons->sch->curLine + 2);
                    else
                       cons->funcAsk(cons, cons->sch->curLine + 1);
                   
                   //Check both First and Last line of the requested range.
                   //Checking just one condition might return FALSE answer due to top/bottom
                   //replication. E.g.:
                   //  expect: 3,4,5,6,6,6
                   //  is    : 4,5,6,6,6,6 : last line seems ok(6), but func_start is overwritten

                   //first & last line(s) sampled by "cons" given KS
                    frst_ln_p = fptr->sch->lineIndices[cons->parentsKS[x] + 0                      ];
                    last_ln_p = fptr->sch->lineIndices[cons->parentsKS[x] + (cons->nLinesUsed[x]-1)];
                   //first & last line(s) requested by "cons"
                    frst_ln_c = cons->sch->reqInLns[x][0                     ];
                    last_ln_c = cons->sch->reqInLns[x][cons->nLinesUsed[x]-1];
 
                    //If consumers OVERWRITE occurs:
                    if( (last_ln_p>last_ln_c) || (frst_ln_p>frst_ln_c) )
                    {
                      #if 1 //This gives better results !!!!!
                         //##########################################################
                         // Buffering on output: if resizer overwrites one consumer,
                         // then let it run, but tell ALL consumers they need to update
                         // in next iteration: we don't clear the KS = -1 as we'd alter
                         // current iteration, but we set a flag, (ks_clr) that gets
                         // interpreted at the beginning of next iteration
                         if( //(fptr->funcSvuRun == RUN_bilin_gen) &&
                             (fptr->flags & SIPP_RESIZE) &&
                             (fptr->sch->curLine < (int)fptr->outputH ) )
                         {
                             UInt32 cc;
                              //fptr->canRunC = RS_CAN_RUN; //this is the default assumption
                              for(cc=0; cc<fptr->nCons; cc++)
                              {
                                UInt32 xx = getParentIndex(cons, fptr);
                                cons->sch->parentsKsClr[xx] = 1;
                              }
                              break;
                         }
                         
                         else{
                             //if cannot run due to a consumer
                             //that's good enough: break !
                             fptr->sch->canRunC = RS_CANNOT;
                             break;
                         }

                      #else 
                        //##########################################################
                        //Buffering on output:
                        // Only care if consumer is NOT a Resizer
                        // as resizers always look for KS all the time anyway...
                        // and so could "safely" overwrite them
                        if(cons->funcSvuRun != RUN_bilin_gen)
                        {
                          //if cannot run due to a consumer
                          //that's good enough to stop current filter: break !
                          fptr->canRunC = RS_CANNOT;
                          break;
                        }
                      #endif
                    } //else, leave RS_CAN_RUN decision

                  }//if we know that consummer CAN or CANNOT run
                }//if tracking consumer

             }//for all consumers

             //RESTORE:
              fptr->sch->curLine--;
              memcpy(fptr->sch->lineIndices, T_indices, sizeof(fptr->sch->lineIndices));
          }//if filt has consumers
         }//if P cond true
        }//for all filters

      }while (allKnown == 0);


    //===================================================================
    // If we reach here, and some filters stat is "DON'T KNOW", we failed !
    //===================================================================
     for(i=0; i<nFilters; i++)
     {
         if((filters[i]->sch->canRunP == RS_DONT_KNOW) ||
            (filters[i]->sch->canRunC == RS_DONT_KNOW)  )
           sippError(E_RUN_DON_T_KNOW);
     }

     //if (pl->dbgLevel > 1) {
     //  sippDbgDumpBuffState(filters, nFilters, iteration);
     //}

    //==============================================================
    //refine a bit rolling decision (bottom replication purposes)
    //==============================================================
    for(i=0; i<nFilters; i++)
    {    
        fptr = filters[i];

        //If a filter is finished,
        if(fptr->sch->curLine >= (int)fptr->outputH-1)
        {
           int all_cons_over = 1;

         //and all consumers are finished, then don't run anymore
           for(c=0; c<fptr->nCons; c++)
           {
               if(fptr->cons[c]->sch->curLine < (int)fptr->cons[c]->outputH-1)
               {
                   all_cons_over = 0;
                   break;
               }
           }

           if(all_cons_over)
               fptr->sch->canRunC = RS_CANNOT;
        }
    }

    
    //=====================================================================
    // Buffer rolling
    //=====================================================================
       for(i=0; i<nFilters; i++)
       {
          if((filters[i]->sch->canRunP == RS_CAN_RUN) && 
             (filters[i]->sch->canRunC == RS_CAN_RUN) )
          {
             filters[i]->sch->curLine++;
             progBuffer(filters[i]); 
          }
       }
    
    //=====================================================================
    // Print runnable tasks:
    //=====================================================================
     if (pl->dbgLevel > 0) {
       sippDbgPrintRunnable(filters, nFilters, iteration);
     }
     sippSchedWr(pl, iteration);

    //=====================================================================
    // See if anything actually ran this iteration
    //=====================================================================
     somethingRuns = 0;
     for(i=0; i<nFilters; i++)
     {
         fptr = filters[i];
         if((fptr->sch->canRunP == RS_CAN_RUN) && 
            (fptr->sch->canRunC == RS_CAN_RUN)  )
         {
            //fptr->funcSvuRun(fptr);
            if(!fptr->sch->dbgJustRoll)
               somethingRuns = 1;
         }
     }

      iteration++;

      if(iteration > pl->schedInfoEntries)
      {
          #if defined(SIPP_PC)
           printf("__ERROR: schedule mem too smal !\n");
           exit(2);
          #endif
      }

      #if defined(SIPP_PC)
       printf("\n");
      #else
       VCS_PRINT_INT(0x1111);
      #endif

   }while(somethingRuns);


   //==================================================
   //If filters didn't reach their run number,s it's failure!
   for(i=0; i<nFilters; i++)
   {
       if(filters[i]->sch->curLine < (int)filters[i]->outputH-1)
       {
         #if defined(SIPP_PC)
          printf("ERROR : filter %d didn't finish! \n", i);
         #else
          VCS_PRINT_INT(0xDEADDEAD);
         #endif

          sippError(3);
       }
   }

   #if defined(SIPP_PC)
    printf("\n\n Finished :) \n\n");
    if (pl->dbgLevel > 0) {
      sippDbgShowBufferReq(filters, nFilters);
    }
   #else
    VCS_PRINT_INT(0x6666);
   #endif
}

//###########################################################################
//This contains 2 steps:
// 1) allocate the fixed header
// 2) allocate the variable header for context switch
void sippAllocRuntimeSched(SippPipeline *pl)
{
  UInt32 max_H, i, u;

 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 //Allocate storage in SIPP-shared pool for scheduler.
 //For this we need to know the largest height in the pipeline, We use this info
 //to guide the allocs. TBD: add checks here !
  max_H = 0;
  for(i=0; i<pl->nFilters; i++)
   if(pl->filters[i]->outputH > max_H)
     max_H = pl->filters[i]->outputH;

  pl->schedInfoEntries = max_H*2;
  pl->schedInfo        = (SchedInfo*)sippMemAlloc(mempool_sipp, pl->schedInfoEntries * sizeof(SchedInfo));
  //WARNING: in scheduler: if exceeding alloced size, should write a message

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  pl->schedInfoCtxSz = 1; //start with 1 as first entry = start/wait mask for multi-ctx units

  for(u=SIPP_RAW_ID; u<=SIPP_DBYR_PPM_ID; u++)
  {
       if(pl->hwSippFltCnt[u]>1)
       {
           pl->hwSippCtxOff[u]  =  pl->schedInfoCtxSz;
           pl->schedInfoCtxSz  += (pl->hwSippFltCnt[u]+1); //alloc one more space for NULL command
       }
  }

  pl->schedInfoCtx = (UInt32*)sippMemAlloc(mempool_sipp, pl->schedInfoEntries * sizeof(pl->schedInfoCtx[0]) * pl->schedInfoCtxSz);
}

//###########################################################################
// Schedule interface
//###########################################################################
void sippSchedPipeline(SippPipeline *pl)
{
  UInt32 i, p;
  SippFilter *fptr;

  pl->nIter = 0;

  //Allocate mem for runtime Scheduler info
  sippAllocRuntimeSched(pl);

 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 //Associate temporarly additional schedule storage 
 //Use CMX as temp storage for this (A few tens of KB shoudl be enough)
  for(i=0; i<pl->nFilters; i++)
  {
    fptr = pl->filters[i];
    fptr->sch = (SippSched*)sippMemAlloc(mempool_sched,sizeof(SippSched));

    //Allocate the line_required arrays for each parent
    for(p=0; p<fptr->nParents; p++)
    {
      fptr->sch->reqInLns[p] = (int*)sippMemAlloc(
                                         mempool_sched, 
                                         sizeof(fptr->sch->reqInLns[0][0]) 
                                         * fptr->nLinesUsed[p]
                                     );
    }
                                                    
    //allow large buffers for first schedule run
    if(fptr->nCons == 0)
    { //if a filter has no consumer, then it doesn't need output buffer
       fptr->nLines = 0;
    }
    else
       fptr->nLines = BUFF_HUGE_SZ;
  }
  
  #if defined(SIPP_PC)
  sippDbgDumpGraph(pl, "pipeline.dot");
  #endif

  //Figure out buffering requirements and the schedule
  simSchedule(pl);
  //rename("auto_sched.txt", "auto_sched_raw.txt");

  //Buffer size minimization:
  minimizeLineBuffs(pl);

  // Overwrite graph with new version, now that line buffer sizes are known
 #if defined(SIPP_PC)
  sippDbgDumpGraph(pl, "pipeline.dot");
 #endif
  
 //Second scheduling iteration: this is just a check basically
 // keep just on PC side (worst case)
 #if defined(SIPP_PC) && 0
  simSchedule(pl);
  //rename("auto_sched.txt", "auto_sched_opt.txt");
 #endif
}
