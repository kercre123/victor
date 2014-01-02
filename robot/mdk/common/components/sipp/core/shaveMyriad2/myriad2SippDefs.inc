;--------------------------------------------------------------
;automatically generated from PC app (sippDbgDumpAsmOffsets)   
;--------------------------------------------------------------

;offsets within SippFilter struct
.set F_O_SVU_RUN         76 ; fptr->funcSvuRun      
.set F_O_PARAMS          80 ; fptr->params          
.set F_O_N_PARENTS       88 ; fptr->nParents        
.set F_O_PARENTS         20 ; fptr->parents         
.set F_O_LINES_I        124 ; fptr->dbLinesIn[0]    
.set F_O_LINES_O        156 ; fptr->dbLineOut[0]    
.set F_O_OUT_W          164 ; fptr->outputW         
.set F_O_SLICE_W        168 ; fptr->sliceWidth      
.set F_O_PL_STRIDE      184 ; fptr->planeStride     
.set F_O_OUT_H           52 ; fptr->outputH         
.set F_O_EXENO          212 ; fptr->exeNo           
.set F_O_COMM_INFO      224 ; fptr->gi              
.set F_O_LPTR_2ND        16 ; fptr->linePtrs2ndBase 
.set F_O_LPTR_3RD        12 ; fptr->linePtrs3rdBase 
.set F_O_OUT_LPTR         4 ; fptr->outLinePtr      
.set F_O_LINES_PTR        8 ; fptr->linePtrs        
.set F_O_N_CONS           0 ; fptr->nCons           
.set F_O_N_LINES        200 ; fptr->nLines          

;offsets within CommInfo struct
.set G_O_FIRST_S          0 ; gi->sliceFirst 
.set G_O_LAST_S           4 ; gi->sliceLast  
.set G_O_SLC_SZ           8 ; gi->sliceSize  
.set G_O_CUR_F           12 ; gi->curFrame   

;offsets within SippPipeline struct
.set P_O_SVU_CMD         28 ; pl->svuCmd     
.set P_O_SVU_LIST       160 ; pl->filtersSvu 
.set P_O_SVU_LIST_SZ    288 ; pl->nFiltersSvu
.set P_O_ITER          1448 ; pl->iteration  
.set P_O_SCHED_NFO     1392 ; pl->schedInfo  
.set P_O_GLOBAL_NFO    1408 ; pl->gi         
.set P_O_SVU_STAT      1452 ; pl->svuStat    
.set P_O_SVU_START_M   1464 ; pl->svuStartMask

;offsets within SchedInfo struct
.set S_O_SVU_MASK         8 ; schedInfo[0].shaveMask
.set SZ_SCH_INFO         20 ; sizeof(pl->schedInfo[0])
