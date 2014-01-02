#include <sipp.h>

#if 0

extern void sipp_alloc_ln_buffs(SippPipeline *pl);
extern void sippDoHorizReplication(SippFilter *fptr, int svuNo);

//################################################################################
void full_frame_run(SippPipeline *pl, SippFilter *fptr)
{
    UInt32 y, p, s, ln, u;
    SippFilter *par;


   //Populate the target execution unit with just one filter
    u = fptr->unit;
    pl->exeU[u].execIdx                = 0;
    pl->exeU[u].dbWorkList->filters[0] = fptr;
    pl->exeU[u].dbWorkList->fill       = 1;

    //======================================================================
    //a) Run the filter to generate all output lines
    for(y=0; y<fptr->outputH; y++)
    {
        //------------------------------------------------
        //1) Find out what input pointers in all parent buffers
        for(p=0; p<fptr->nParents; p++)
        {
            //Find out what line indices are needed
            fptr->funcAsk(fptr, y);

            //Compute pointers to those lines...
            par = fptr->parents[p];
            //UInt8 *dbLinesIn[MAX_PARENTS][2][15];
            for(ln=0; ln<fptr->nLinesUsed[p]; ln++)
            {
               fptr->dbLinesIn[p][y&1][ln] = par->outputBuffer
                                             + (fptr->sch->reqInLns[p][ln] * par->lineStride + par->hPadding) * par->bpp;
            }
        }

        //------------------------------------------------
        //2) Set output pointer:
        if(fptr->nCons)
        {
            //fptr->lineStride  = fptr->bpp * (fptr->sliceWidth + fptr->hPadding * 2);
            fptr->dbLineOut[y&1] = fptr->outputBuffer
                                     + (y * fptr->lineStride + fptr->hPadding) * fptr->bpp;
        }

        //Auch, unit-kicks... huh. need to rework this part !!!!
        fptr->lnToPad[y&1] = NULL;//this cancels the h-padding
        pl->exeU[u].kick(&pl->exeU[u]);

        //------------------------------------------------
        //The line padding:
         if(fptr->hPadding)
         {
            fptr->exeNo = y;
            fptr->lnToPad[y&1] = fptr->dbLineOut[y&1];

            for(s = pl->sliceFirst; s <= pl->sliceLast; s++)
              sippDoHorizReplication(fptr, s);
         }
        //..TBD
    }

    //======================================================================
    //b) Current filter is done, IF it has output buffer, save it to a file
    if(fptr->nCons)
    {
        char fName[256];
        FILE *f;

        sprintf(fName, "full_frm_FILT_%02d.raw", fptr->id);
        f = fopen(fName, "wb");

        for(y=0; y<fptr->outputH; y++)
        {
          for(s = pl->sliceFirst; s <= pl->sliceLast; s++)
          {
             //ALU: save this as U8 gizas if buffer is FP16 !!!
             fwrite( WRESOLVE(fptr->outputBuffer + (y * fptr->lineStride + fptr->hPadding) * fptr->bpp, s),
                     fptr->bpp, 
                     fptr->sliceWidth, 
                     f );
          }
        }
        fclose(f);
    }

    fptr->exeNo = fptr->outputH;  //mark we finished run
}

//################################################################################
void sipp_frame_check(SippPipeline *pl)
{
   UInt32 i, p, did_something;
   SippSched tmp[100]; //temp storage (hogs stack... should improve this)

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //1) Allocate full-frame-buffers for all filters that have output buffers
  //   The alloc is done per slice...
    for(i=0;  i<pl->nFilters; i++)
    {
        pl->filters[i]->exeNo = 0;      //mark we didn't run yet
        pl->filters[i]->sch    = &tmp[i];//alloc some temp buffer (on stack for now)

        if(pl->filters[i]->nCons)
        {
            pl->filters[i]->nLines = pl->filters[i]->outputH - 1;
        }
    }

    sipp_alloc_ln_buffs(pl);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //2)For all filters that have completed parents, execute....
  //  while there's nothing else to do...
   do{
       did_something = 0;

       //loop through all filters: if we find a filter who's parents are all DONE
       //and current parent didn't run, then RUN curr filter
       for(i=0;  i<pl->nFilters; i++)
       {
           //assume parents are done
           //a filter without parents, will be albe to run straignt away
           int all_parents_done = 1;
                                    
           for(p=0; p<pl->filters[i]->nParents; p++)
           {
               if(pl->filters[i]->parents[p]->exeNo == 0)
               {//if at least one parent isn't over yet, then
                //condition becomes FALSE, so can't run
                   all_parents_done = 0;
                   break;
               }
           }

           //If filter has all full-frame parent data ready, and didn't run
           //yet, then run now
           if(all_parents_done && (pl->filters[i]->exeNo == 0))
           {
               did_something = 1;
               full_frame_run(pl, pl->filters[i]);
               break;
           }
       }

   }while(did_something);

   printf("__FRAME_CHECK_COMPLETE__\n");
}

#endif
