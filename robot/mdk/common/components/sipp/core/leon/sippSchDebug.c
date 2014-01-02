#include <sipp.h>

//##############################################################################################
//##############################################################################################
void sippSchedWr(SippPipeline *pl, UInt32 iteration)
{
    SippFilter *fptr;
    UInt32 i;
    UInt32 maskSipp;
    UInt32 maskShave;
    UInt32 maskDMA;

#if defined(__sparc) || defined (SIPP_PC)
   //Clear mask
    pl->schedInfo[iteration].allMask = 0; 

   //Set all bits for the filters that need to run
    for(i=0; i<pl->nFilters; i++)
    {
        fptr = pl->filters[i];
        if((fptr->sch->canRunP == RS_CAN_RUN) && 
           (fptr->sch->canRunC == RS_CAN_RUN)  )
             pl->schedInfo[iteration].allMask |= (1<<i);
    }

   
   //New Scheduler, add progresively and regress...

   //0) All filters masks
//    pl->schedInfo[iteration].ksUpdates = NULL;

   //================================================================
   //1)Build the SIPP Start mask
    maskSipp  = 0;
    
    //for sabre, mask stays 0 forever...
    #if defined(SIPP_PC) || defined(MYRIAD2)
    for(i=0; i<pl->nFilters; i++)
    {
        fptr = pl->filters[i];

        //If a filter is to RUN, and is a HW filter running in SINGLE-CTX mode,
        //update corresponding bit. Multi-CTX units are started differently, via sippHandleCtxUnit
        if(   (pl->schedInfo[iteration].allMask & (1<<i)) // if filter is to run
            &&(fptr->sch->curLine < fptr->outputH)        // and didn't finish output frame
            &&(pl->hwSippFltCnt[fptr->unit] <= 1) )       // and runs on SINGLE-context unit
        {
            switch(fptr->unit)
            {
                case SIPP_MED_ID     : maskSipp |= (1<<SIPP_MED_ID    ); break;
                case SIPP_CONV_ID    : maskSipp |= (1<<SIPP_CONV_ID   ); break;
                case SIPP_LUT_ID     : maskSipp |= (1<<SIPP_LUT_ID    ); break;
                case SIPP_LSC_ID     : maskSipp |= (1<<SIPP_LSC_ID    ); break;
                case SIPP_RAW_ID     : maskSipp |= (1<<SIPP_RAW_ID    ); break;
                case SIPP_DBYR_ID    : maskSipp |= (1<<SIPP_DBYR_ID   ); break;
                case SIPP_CHROMA_ID  : maskSipp |= (1<<SIPP_CHROMA_ID ); break;
                case SIPP_LUMA_ID    : maskSipp |= (1<<SIPP_LUMA_ID   ); break;
                case SIPP_SHARPEN_ID : maskSipp |= (1<<SIPP_SHARPEN_ID); break;
                case SIPP_DBYR_PPM_ID: maskSipp |= (1<<SIPP_DBYR_PPM_ID);break;
                case SIPP_HARRIS_ID  : maskSipp |= (1<<SIPP_HARRIS_ID ); break;
                case SIPP_UPFIRDN_ID : maskSipp |= (1<<SIPP_UPFIRDN_ID); break;
                case SIPP_CC_ID      : maskSipp |= (1<<SIPP_CC_ID     ); break;
                case SIPP_EDGE_OP_ID : maskSipp |= (1<<SIPP_EDGE_OP_ID); break;
            }
        }
    }
    #endif
    pl->schedInfo[iteration].sippHwStartMask = maskSipp;
    pl->schedInfo[iteration].sippHwWaitMask  = maskSipp;

    //================================================================
    //Shave masks : parse Shave task lists
    maskShave = 0;
    for(i=0; i<pl->nFiltersSvu; i++)
    {
        fptr = pl->filtersSvu[i];

        if( (fptr->sch->canRunP == RS_CAN_RUN) && 
            (fptr->sch->canRunC == RS_CAN_RUN) &&
            (fptr->sch->curLine < fptr->outputH) )
        {
            maskShave|= (1<<i);
        }
    }
    pl->schedInfo[iteration].shaveMask = maskShave;

    //================================================================
    //DMA mask
    maskDMA = 0;
    for(i=0; i<pl->nFiltersDMA; i++)
    {
        fptr = pl->filtersDMA[i];

        if( (fptr->sch->canRunP == RS_CAN_RUN) && 
            (fptr->sch->canRunC == RS_CAN_RUN) &&
            (fptr->sch->curLine < fptr->outputH) )
        {
            maskDMA|= (1<<i);
        }
    }
    pl->schedInfo[iteration].dmaMask = maskDMA;

   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    pl->nIter = iteration;

#else //SIPP_PC
    FILE     *f;
    char     bS, bF;
    UInt32 p;
    
    if(iteration == 0) f = fopen("auto_sched.txt", "w");
    else               f = fopen("auto_sched.txt", "a");

    fprintf(f, "%04d : ", iteration);

    for(i=0; i<pl->nFilters; i++)
    {
          fptr = pl->filters[i];

          if((fptr->sch->canRunP == RS_CAN_RUN) && 
             (fptr->sch->canRunC == RS_CAN_RUN)  )
          {
            //figure out brackets
            if(fptr->sch->dbgJustRoll)  { bS = '('; bF = ')'; }
            else                          { bS = '['; bF = ']'; }
   
            fprintf(f,"%c%2d", bS, i);
            //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            //Kernel starts: for filters with parents
            if(fptr->nParents)
            {
                fprintf(f,":");
                for(p=0; p<fptr->nParents; p++)
                {
                    fprintf(f,"%2d", fptr->parentsKS[p]);
                    if(p < fptr->nParents -1)
                        fprintf(f,",");
                }
            }
            //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            fprintf(f, "%c ", bF);

            
          }

          else{ //If can't run, just print spaces instead of filter info...
              fprintf(f,"     ");//for the 2xbrackets, filter num %2 and a space

              if(fptr->nParents)
              {
                  fprintf(f," "); //for :
                  for(p=0; p<fptr->nParents;   p++) fprintf(f,"  ");//for KS %2
                  for(p=0; p<fptr->nParents-1; p++) fprintf(f," "); //for ","
              }
            
          }
    }
    fprintf(f, "\n");
    fclose(f);
#endif
}


//#############################################################################
void sippDbgPrintRunnable(SippFilter *filters[], UInt32 nFilters, UInt32 iteration)
{
   #if defined(SIPP_PC)
    UInt32      i, p;
    SippFilter *fptr;
    char          bS, bF;

    printf("%04d : ", iteration);
    for(i=0; i<nFilters; i++)
    {
          fptr = filters[i];

          if((fptr->sch->canRunP == RS_CAN_RUN) && 
             (fptr->sch->canRunC == RS_CAN_RUN)  )
          {
            //figure out brackets
            if(fptr->sch->dbgJustRoll)  { bS = '('; bF = ')'; }
            else                          { bS = '['; bF = ']'; }
   
            printf("%c%3d", bS, i);
            //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            //Kernel starts: for filters with parents
            if(fptr->nParents)
            {
                printf(":");
                for(p=0; p<fptr->nParents; p++)
                {
                    printf("%3d", fptr->parentsKS[p]);
                    if(p < fptr->nParents -1)
                        printf(",");
                }
            }
            //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            printf("%c ", bF);

            
          }

          else{ //If can't run, just print spaces instead of filter info...
              printf("      ");//for the 2xbrackets, filter num %3 and a space

              if(fptr->nParents)
              {
                  printf(" "); //for :
                  for(p=0; p<fptr->nParents;   p++) printf("   ");//for KS %3
                  for(p=0; p<fptr->nParents-1; p++) printf(" "); //for ","
              }
            
          }
    }
  #endif
}

//#############################################################################
//Display number of lines in each buffer and KernelStarts in parent buffers
//#############################################################################
void sippDbgShowBufferReq(SippFilter **filters, UInt32 nFilters)
{
  #if defined(SIPP_PC)
   UInt32 i, p;
   
   printf("\n\n======================================\n");
   printf(" Buff size, Kernel Starts required (will need +1):   \n");
   printf("======================================\n");
   for(i=0;i<nFilters;i++)
   {
      printf("Filt %2d : ", i);
      printf("Num_lines = %2d  ", filters[i]->nLines);
      printf("Kernel_start: ");
      for(p=0;p<filters[i]->nParents;p++)
      {
          printf("%2d,", filters[i]->sch->parentsKsMin[p]);
      }
      printf("\n");
   }
  #endif
}

////#############################################################################
//// Dump the lineIndices buffers for all filters here, each column in output
//// file corresponds to an iteration
////#############################################################################
//void sippDbgDumpBuffState(SippFilter *filters[], UInt32 nFilters, UInt32 iteration)
//{
//  #if defined(SIPP_PC)
//    FILE *f, *g;
//    UInt32 i,l;
//    char old_line[1024];
//
//    if(iteration == 0)
//    {
//      remove("global_progress.txt");
//      //create empty file
//      f = fopen("global_progress.txt", "w");
//      for(i=0; i<nFilters; i++)
//      {
//          for(l=0; l<filters[i]->nLines; l++)
//             fprintf(f,"filt_%04d : \n", i);
//           fprintf(f,"============\n");
//      }
//    
//      fclose(f);
//    }
//
//    f = fopen("global_progress.txt", "r");
//    g = fopen("global_temp.txt",     "w");
//
//    for(i=0; i<nFilters; i++)
//    {
//        
//        for(l=0; l<filters[i]->nLines; l++)
//        {
//          if(fgets(old_line, sizeof(old_line), f)){
//             old_line[strlen(old_line)-1] = 0; //gizas....
//             fputs(old_line, g);
//          }
//
//          if(filters[i]->sch->lineIndices[l] != -1)
//          {
//            fprintf(g, "%4d|\n", filters[i]->sch->lineIndices[l]);
//            fflush(g);
//          }
//          else{
//            fprintf(g, "xxxx|\n");
//            fflush(g);
//          }
//        }
//
//        //the line separator
//        if(fgets(old_line, sizeof(old_line), f))
//        {
//           old_line[strlen(old_line)-1] = 0;
//           fputs(old_line, g);
//        }
//
//        fprintf(g,"=====\n");
//    }
//    
//    fclose(f);
//    fclose(g);
//
//    remove("global_progress.txt");
//    rename("global_temp.txt", "global_progress.txt");
//  #endif
//}

////#############################################################################
//void DBG_print_num_par(SippFilter *filters[], UInt32 nFilters)
//{
//    UInt32 n_par_links = 0;
//    UInt32 i;
//
//    for(i=0; i<nFilters; i++)
//        n_par_links += filters[i]->nParents;
//
//    printf("__Number of parent links = %d__ \n", n_par_links);
//}
