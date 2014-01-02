/********************************************************************************************************************
   ------------------------------------------------------------------------------------------------------------------
    Sections included from misc.lds.h
    - This block defines the placement of:
       dram code/data (TODO: Move to own file)
       TODO: Describe the rest of this stuff
   ------------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/

/*General purpose sections to be used by anyone who needs them*/

SECTIONS {
  /* defining DDR sections for use through L2 cache (I.E. NOT "_direct") */
  . = 0x80000000;
  .ddr.text : {  *(.ddr.text*)  
		 *(.lrt.ddr.text*)   
		}
  .ddr.rodata : { *(.ddr.rodata*) 
		  *(.lrt.ddr.rodata*)  
		}
  .ddr.data : { KEEP(*(.ddr.data*)) 
		KEEP(*(.lrt.ddr.data*))  
	      }
  .ddr.bss (NOLOAD) : {
    __ddr_bss_start = ALIGN(8);
     *(.ddr.bss*)
     *(.lrt.ddr.bss*)
    __ddr_bss_end = ALIGN(8);
  }
  __END_OF_L2_ACCESS_SECTIONS = ALIGN(4);

  /*Tentative definitions for people not using the myriad1_shave_slices.ldscript*/  
  __NEXT_FREE_CMX_SLICE = DEFINED( __NEXT_FREE_CMX_SLICE ) ? __NEXT_FREE_CMX_SLICE : ABSOLUTE(0x70000000);
  
  . = __NEXT_FREE_CMX_SLICE;
  .cmx.text : { *(.cmx.text*) }
  .cmx.rodata : { *(.cmx.rodata*) }
  .cmx.data : { KEEP(*(.cmx.data*)) }
  .cmx.bss (NOLOAD) : { 
    __cmx_bss_start = ALIGN(8);
      *(.cmx.bss*) 
    __cmx_bss_end = ALIGN(8);
  }


  CMX_MEMORY_END = . ;
  /*Check if the end of CMX was achieved and don't link if so*/
  ASSERT( CMX_MEMORY_END < 0x70200000 , "CMX size is exceeded! Please move code and/or data from CMX.")
    
  /* This section allows the user to optionally overide the stack pointer from the default value using a custom.ldscript*/
  __SVE0_STACK_POINTER  = DEFINED( __SVE0_USER_STACK_POINTER)  ? __SVE0_USER_STACK_POINTER  : __CMX_SLICE1_START ; 
  __SVE1_STACK_POINTER  = DEFINED( __SVE1_USER_STACK_POINTER)  ? __SVE1_USER_STACK_POINTER  : __CMX_SLICE2_START ; 
  __SVE2_STACK_POINTER  = DEFINED( __SVE2_USER_STACK_POINTER)  ? __SVE2_USER_STACK_POINTER  : __CMX_SLICE3_START ; 
  __SVE3_STACK_POINTER  = DEFINED( __SVE3_USER_STACK_POINTER)  ? __SVE3_USER_STACK_POINTER  : __CMX_SLICE4_START ; 
  __SVE4_STACK_POINTER  = DEFINED( __SVE4_USER_STACK_POINTER)  ? __SVE4_USER_STACK_POINTER  : __CMX_SLICE5_START ; 
  __SVE5_STACK_POINTER  = DEFINED( __SVE5_USER_STACK_POINTER)  ? __SVE5_USER_STACK_POINTER  : __CMX_SLICE6_START ; 
  __SVE6_STACK_POINTER  = DEFINED( __SVE6_USER_STACK_POINTER)  ? __SVE6_USER_STACK_POINTER  : __CMX_SLICE7_START ; 
  __SVE7_STACK_POINTER  = DEFINED( __SVE7_USER_STACK_POINTER)  ? __SVE7_USER_STACK_POINTER  : __CMX_SLICE8_START ; 
  __SVE8_STACK_POINTER  = DEFINED( __SVE8_USER_STACK_POINTER)  ? __SVE8_USER_STACK_POINTER  : __CMX_SLICE9_START ; 
  __SVE9_STACK_POINTER  = DEFINED( __SVE9_USER_STACK_POINTER)  ? __SVE9_USER_STACK_POINTER  : __CMX_SLICE10_START; 
  __SVE10_STACK_POINTER = DEFINED( __SVE10_USER_STACK_POINTER) ? __SVE10_USER_STACK_POINTER : __CMX_SLICE11_START; 
  __SVE11_STACK_POINTER = DEFINED( __SVE11_USER_STACK_POINTER) ? __SVE11_USER_STACK_POINTER : __CMX_END          ; 

  /* Leon RT Version of Stack pointer */
  lrt___SVE0_STACK_POINTER  = DEFINED( lrt___SVE0_USER_STACK_POINTER)  ? lrt___SVE0_USER_STACK_POINTER  : __CMX_SLICE1_START ; 
  lrt___SVE1_STACK_POINTER  = DEFINED( lrt___SVE1_USER_STACK_POINTER)  ? lrt___SVE1_USER_STACK_POINTER  : __CMX_SLICE2_START ; 
  lrt___SVE2_STACK_POINTER  = DEFINED( lrt___SVE2_USER_STACK_POINTER)  ? lrt___SVE2_USER_STACK_POINTER  : __CMX_SLICE3_START ; 
  lrt___SVE3_STACK_POINTER  = DEFINED( lrt___SVE3_USER_STACK_POINTER)  ? lrt___SVE3_USER_STACK_POINTER  : __CMX_SLICE4_START ; 
  lrt___SVE4_STACK_POINTER  = DEFINED( lrt___SVE4_USER_STACK_POINTER)  ? lrt___SVE4_USER_STACK_POINTER  : __CMX_SLICE5_START ; 
  lrt___SVE5_STACK_POINTER  = DEFINED( lrt___SVE5_USER_STACK_POINTER)  ? lrt___SVE5_USER_STACK_POINTER  : __CMX_SLICE6_START ; 
  lrt___SVE6_STACK_POINTER  = DEFINED( lrt___SVE6_USER_STACK_POINTER)  ? lrt___SVE6_USER_STACK_POINTER  : __CMX_SLICE7_START ; 
  lrt___SVE7_STACK_POINTER  = DEFINED( lrt___SVE7_USER_STACK_POINTER)  ? lrt___SVE7_USER_STACK_POINTER  : __CMX_SLICE8_START ; 
  lrt___SVE8_STACK_POINTER  = DEFINED( lrt___SVE8_USER_STACK_POINTER)  ? lrt___SVE8_USER_STACK_POINTER  : __CMX_SLICE9_START ; 
  lrt___SVE9_STACK_POINTER  = DEFINED( lrt___SVE9_USER_STACK_POINTER)  ? lrt___SVE9_USER_STACK_POINTER  : __CMX_SLICE10_START; 
  lrt___SVE10_STACK_POINTER = DEFINED( lrt___SVE10_USER_STACK_POINTER) ? lrt___SVE10_USER_STACK_POINTER : __CMX_SLICE11_START; 
  lrt___SVE11_STACK_POINTER = DEFINED( lrt___SVE11_USER_STACK_POINTER) ? lrt___SVE11_USER_STACK_POINTER : __CMX_END          ; 


  /* Define linker symbols with desired location for windows C,D,E,F on each shave */  
  __WinRegShave0_winC = DEFINED( __WinRegShave0_winC ) ? __WinRegShave0_winC : ABSOLUTE(0x70008000);    /* Default absolute address  */
  __WinRegShave0_winD = DEFINED( __WinRegShave0_winD ) ? __WinRegShave0_winD : ABSOLUTE(0x70000000);    /* Default absolute address  */
  __WinRegShave0_winE = DEFINED( __WinRegShave0_winE ) ? __WinRegShave0_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave0_winF = DEFINED( __WinRegShave0_winF ) ? __WinRegShave0_winF : ABSOLUTE(0x40000000);    /* not used                  */

  __WinRegShave1_winC = DEFINED( __WinRegShave1_winC ) ? __WinRegShave1_winC : ABSOLUTE(0x70028000);    /* Default absolute address  */
  __WinRegShave1_winD = DEFINED( __WinRegShave1_winD ) ? __WinRegShave1_winD : ABSOLUTE(0x70020000);    /* Default absolute address  */
  __WinRegShave1_winE = DEFINED( __WinRegShave1_winE ) ? __WinRegShave1_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave1_winF = DEFINED( __WinRegShave1_winF ) ? __WinRegShave1_winF : ABSOLUTE(0x40000000);    /* not used                  */
  
  __WinRegShave2_winC = DEFINED( __WinRegShave2_winC ) ? __WinRegShave2_winC : ABSOLUTE(0x70048000);    /* Default absolute address  */
  __WinRegShave2_winD = DEFINED( __WinRegShave2_winD ) ? __WinRegShave2_winD : ABSOLUTE(0x70040000);    /* Default absolute address  */
  __WinRegShave2_winE = DEFINED( __WinRegShave2_winE ) ? __WinRegShave2_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave2_winF = DEFINED( __WinRegShave2_winF ) ? __WinRegShave2_winF : ABSOLUTE(0x40000000);    /* not used                  */

  __WinRegShave3_winC = DEFINED( __WinRegShave3_winC ) ? __WinRegShave3_winC : ABSOLUTE(0x70068000);    /* Default absolute address  */
  __WinRegShave3_winD = DEFINED( __WinRegShave3_winD ) ? __WinRegShave3_winD : ABSOLUTE(0x70060000);    /* Default absolute address  */
  __WinRegShave3_winE = DEFINED( __WinRegShave3_winE ) ? __WinRegShave3_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave3_winF = DEFINED( __WinRegShave3_winF ) ? __WinRegShave3_winF : ABSOLUTE(0x40000000);    /* not used                  */
                                                 
  __WinRegShave4_winC = DEFINED( __WinRegShave4_winC ) ? __WinRegShave4_winC : ABSOLUTE(0x70088000);    /* Default absolute address  */
  __WinRegShave4_winD = DEFINED( __WinRegShave4_winD ) ? __WinRegShave4_winD : ABSOLUTE(0x70080000);    /* Default absolute address  */
  __WinRegShave4_winE = DEFINED( __WinRegShave4_winE ) ? __WinRegShave4_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave4_winF = DEFINED( __WinRegShave4_winF ) ? __WinRegShave4_winF : ABSOLUTE(0x40000000);    /* not used                  */

  __WinRegShave5_winC = DEFINED( __WinRegShave5_winC ) ? __WinRegShave5_winC : ABSOLUTE(0x700A8000);    /* Default absolute address  */
  __WinRegShave5_winD = DEFINED( __WinRegShave5_winD ) ? __WinRegShave5_winD : ABSOLUTE(0x700A0000);    /* Default absolute address  */
  __WinRegShave5_winE = DEFINED( __WinRegShave5_winE ) ? __WinRegShave5_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave5_winF = DEFINED( __WinRegShave5_winF ) ? __WinRegShave5_winF : ABSOLUTE(0x40000000);    /* not used                  */
                                                 
  __WinRegShave6_winC = DEFINED( __WinRegShave6_winC ) ? __WinRegShave6_winC : ABSOLUTE(0x700C8000);    /* Default absolute address  */
  __WinRegShave6_winD = DEFINED( __WinRegShave6_winD ) ? __WinRegShave6_winD : ABSOLUTE(0x700C0000);    /* Default absolute address  */
  __WinRegShave6_winE = DEFINED( __WinRegShave6_winE ) ? __WinRegShave6_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave6_winF = DEFINED( __WinRegShave6_winF ) ? __WinRegShave6_winF : ABSOLUTE(0x40000000);    /* not used                  */
                                                 
  __WinRegShave7_winC = DEFINED( __WinRegShave7_winC ) ? __WinRegShave7_winC : ABSOLUTE(0x700E8000);    /* Default absolute address  */
  __WinRegShave7_winD = DEFINED( __WinRegShave7_winD ) ? __WinRegShave7_winD : ABSOLUTE(0x700E0000);    /* Default absolute address  */
  __WinRegShave7_winE = DEFINED( __WinRegShave7_winE ) ? __WinRegShave7_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave7_winF = DEFINED( __WinRegShave7_winF ) ? __WinRegShave7_winF : ABSOLUTE(0x40000000);    /* not used                  */
  
  __WinRegShave8_winC = DEFINED( __WinRegShave8_winC ) ? __WinRegShave8_winC : ABSOLUTE(0x70108000);    /* Default absolute address  */
  __WinRegShave8_winD = DEFINED( __WinRegShave8_winD ) ? __WinRegShave8_winD : ABSOLUTE(0x70100000);    /* Default absolute address  */
  __WinRegShave8_winE = DEFINED( __WinRegShave8_winE ) ? __WinRegShave8_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave8_winF = DEFINED( __WinRegShave8_winF ) ? __WinRegShave8_winF : ABSOLUTE(0x80000000);    /* not used                  */
  
  __WinRegShave9_winC = DEFINED( __WinRegShave9_winC ) ? __WinRegShave9_winC : ABSOLUTE(0x70128000);    /* Default absolute address  */
  __WinRegShave9_winD = DEFINED( __WinRegShave9_winD ) ? __WinRegShave9_winD : ABSOLUTE(0x70120000);    /* Default absolute address  */
  __WinRegShave9_winE = DEFINED( __WinRegShave9_winE ) ? __WinRegShave9_winE : ABSOLUTE(0x00000000);    /* not used                  */
  __WinRegShave9_winF = DEFINED( __WinRegShave9_winF ) ? __WinRegShave9_winF : ABSOLUTE(0x80000000);    /* not used                  */
  
  __WinRegShave10_winC = DEFINED( __WinRegShave10_winC ) ? __WinRegShave10_winC : ABSOLUTE(0x70148000); /* Default absolute address  */
  __WinRegShave10_winD = DEFINED( __WinRegShave10_winD ) ? __WinRegShave10_winD : ABSOLUTE(0x70140000); /* Default absolute address  */
  __WinRegShave10_winE = DEFINED( __WinRegShave10_winE ) ? __WinRegShave10_winE : ABSOLUTE(0x00000000); /* not used                  */
  __WinRegShave10_winF = DEFINED( __WinRegShave10_winF ) ? __WinRegShave10_winF : ABSOLUTE(0x80000000); /* not used                  */
  
  __WinRegShave11_winC = DEFINED( __WinRegShave11_winC ) ? __WinRegShave11_winC : ABSOLUTE(0x70168000); /* Default absolute address  */
  __WinRegShave11_winD = DEFINED( __WinRegShave11_winD ) ? __WinRegShave11_winD : ABSOLUTE(0x70160000); /* Default absolute address  */
  __WinRegShave11_winE = DEFINED( __WinRegShave11_winE ) ? __WinRegShave11_winE : ABSOLUTE(0x00000000); /* not used                  */
  __WinRegShave11_winF = DEFINED( __WinRegShave11_winF ) ? __WinRegShave11_winF : ABSOLUTE(0x80000000); /* not used                  */
  
  __WinRegShave12_winC = DEFINED( __WinRegShave12_winC ) ? __WinRegShave12_winC : ABSOLUTE(0x70188000); /* Default absolute address  */
  __WinRegShave12_winD = DEFINED( __WinRegShave12_winD ) ? __WinRegShave12_winD : ABSOLUTE(0x70180000); /* Default absolute address  */
  __WinRegShave12_winE = DEFINED( __WinRegShave12_winE ) ? __WinRegShave12_winE : ABSOLUTE(0x00000000); /* not used                  */
  __WinRegShave12_winF = DEFINED( __WinRegShave12_winF ) ? __WinRegShave12_winF : ABSOLUTE(0x80000000); /* not used                  */
  
  __WinRegShave13_winC = DEFINED( __WinRegShave13_winC ) ? __WinRegShave13_winC : ABSOLUTE(0x701A8000); /* Default absolute address  */
  __WinRegShave13_winD = DEFINED( __WinRegShave13_winD ) ? __WinRegShave13_winD : ABSOLUTE(0x701A0000); /* Default absolute address  */
  __WinRegShave13_winE = DEFINED( __WinRegShave13_winE ) ? __WinRegShave13_winE : ABSOLUTE(0x00000000); /* not used                  */
  __WinRegShave13_winF = DEFINED( __WinRegShave13_winF ) ? __WinRegShave13_winF : ABSOLUTE(0x80000000); /* not used                  */
  
  __WinRegShave14_winC = DEFINED( __WinRegShave14_winC ) ? __WinRegShave14_winC : ABSOLUTE(0x701C8000); /* Default absolute address  */
  __WinRegShave14_winD = DEFINED( __WinRegShave14_winD ) ? __WinRegShave14_winD : ABSOLUTE(0x701C0000); /* Default absolute address  */
  __WinRegShave14_winE = DEFINED( __WinRegShave14_winE ) ? __WinRegShave14_winE : ABSOLUTE(0x00000000); /* not used                  */
  __WinRegShave14_winF = DEFINED( __WinRegShave14_winF ) ? __WinRegShave14_winF : ABSOLUTE(0x80000000); /* not used                  */
  
  __WinRegShave15_winC = DEFINED( __WinRegShave15_winC ) ? __WinRegShave15_winC : ABSOLUTE(0x701E8000); /* Default absolute address  */
  __WinRegShave15_winD = DEFINED( __WinRegShave15_winD ) ? __WinRegShave15_winD : ABSOLUTE(0x701E0000); /* Default absolute address  */
  __WinRegShave15_winE = DEFINED( __WinRegShave15_winE ) ? __WinRegShave15_winE : ABSOLUTE(0x00000000); /* not used                  */
  __WinRegShave15_winF = DEFINED( __WinRegShave15_winF ) ? __WinRegShave15_winF : ABSOLUTE(0x80000000); /* not used                  */




  /* Define linker symbols with desired location for windows C,D,E,F on each shave */  
  lrt___WinRegShave0_winC = DEFINED( lrt___WinRegShave0_winC ) ? lrt___WinRegShave0_winC : ABSOLUTE(0x70008000);    /* Default absolute address  */
  lrt___WinRegShave0_winD = DEFINED( lrt___WinRegShave0_winD ) ? lrt___WinRegShave0_winD : ABSOLUTE(0x70000000);    /* Default absolute address  */
  lrt___WinRegShave0_winE = DEFINED( lrt___WinRegShave0_winE ) ? lrt___WinRegShave0_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave0_winF = DEFINED( lrt___WinRegShave0_winF ) ? lrt___WinRegShave0_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave1_winC = DEFINED( lrt___WinRegShave1_winC ) ? lrt___WinRegShave1_winC : ABSOLUTE(0x70028000);    /* Default absolute address  */
  lrt___WinRegShave1_winD = DEFINED( lrt___WinRegShave1_winD ) ? lrt___WinRegShave1_winD : ABSOLUTE(0x70020000);    /* Default absolute address  */
  lrt___WinRegShave1_winE = DEFINED( lrt___WinRegShave1_winE ) ? lrt___WinRegShave1_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave1_winF = DEFINED( lrt___WinRegShave1_winF ) ? lrt___WinRegShave1_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave2_winC = DEFINED( lrt___WinRegShave2_winC ) ? lrt___WinRegShave2_winC : ABSOLUTE(0x70048000);    /* Default absolute address  */
  lrt___WinRegShave2_winD = DEFINED( lrt___WinRegShave2_winD ) ? lrt___WinRegShave2_winD : ABSOLUTE(0x70040000);    /* Default absolute address  */
  lrt___WinRegShave2_winE = DEFINED( lrt___WinRegShave2_winE ) ? lrt___WinRegShave2_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave2_winF = DEFINED( lrt___WinRegShave2_winF ) ? lrt___WinRegShave2_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave3_winC = DEFINED( lrt___WinRegShave3_winC ) ? lrt___WinRegShave3_winC : ABSOLUTE(0x70068000);    /* Default absolute address  */
  lrt___WinRegShave3_winD = DEFINED( lrt___WinRegShave3_winD ) ? lrt___WinRegShave3_winD : ABSOLUTE(0x70060000);    /* Default absolute address  */
  lrt___WinRegShave3_winE = DEFINED( lrt___WinRegShave3_winE ) ? lrt___WinRegShave3_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave3_winF = DEFINED( lrt___WinRegShave3_winF ) ? lrt___WinRegShave3_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave4_winC = DEFINED( lrt___WinRegShave4_winC ) ? lrt___WinRegShave4_winC : ABSOLUTE(0x70088000);    /* Default absolute address  */
  lrt___WinRegShave4_winD = DEFINED( lrt___WinRegShave4_winD ) ? lrt___WinRegShave4_winD : ABSOLUTE(0x70080000);    /* Default absolute address  */
  lrt___WinRegShave4_winE = DEFINED( lrt___WinRegShave4_winE ) ? lrt___WinRegShave4_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave4_winF = DEFINED( lrt___WinRegShave4_winF ) ? lrt___WinRegShave4_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave5_winC = DEFINED( lrt___WinRegShave5_winC ) ? lrt___WinRegShave5_winC : ABSOLUTE(0x700A8000);    /* Default absolute address  */
  lrt___WinRegShave5_winD = DEFINED( lrt___WinRegShave5_winD ) ? lrt___WinRegShave5_winD : ABSOLUTE(0x700A0000);    /* Default absolute address  */
  lrt___WinRegShave5_winE = DEFINED( lrt___WinRegShave5_winE ) ? lrt___WinRegShave5_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave5_winF = DEFINED( lrt___WinRegShave5_winF ) ? lrt___WinRegShave5_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave6_winC = DEFINED( lrt___WinRegShave6_winC ) ? lrt___WinRegShave6_winC : ABSOLUTE(0x700C8000);    /* Default absolute address  */
  lrt___WinRegShave6_winD = DEFINED( lrt___WinRegShave6_winD ) ? lrt___WinRegShave6_winD : ABSOLUTE(0x700C0000);    /* Default absolute address  */
  lrt___WinRegShave6_winE = DEFINED( lrt___WinRegShave6_winE ) ? lrt___WinRegShave6_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave6_winF = DEFINED( lrt___WinRegShave6_winF ) ? lrt___WinRegShave6_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave7_winC = DEFINED( lrt___WinRegShave7_winC ) ? lrt___WinRegShave7_winC : ABSOLUTE(0x700E8000);    /* Default absolute address  */
  lrt___WinRegShave7_winD = DEFINED( lrt___WinRegShave7_winD ) ? lrt___WinRegShave7_winD : ABSOLUTE(0x700E0000);    /* Default absolute address  */
  lrt___WinRegShave7_winE = DEFINED( lrt___WinRegShave7_winE ) ? lrt___WinRegShave7_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave7_winF = DEFINED( lrt___WinRegShave7_winF ) ? lrt___WinRegShave7_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave8_winC = DEFINED( lrt___WinRegShave8_winC ) ? lrt___WinRegShave8_winC : ABSOLUTE(0x70108000);    /* Default absolute address  */
  lrt___WinRegShave8_winD = DEFINED( lrt___WinRegShave8_winD ) ? lrt___WinRegShave8_winD : ABSOLUTE(0x70100000);    /* Default absolute address  */
  lrt___WinRegShave8_winE = DEFINED( lrt___WinRegShave8_winE ) ? lrt___WinRegShave8_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave8_winF = DEFINED( lrt___WinRegShave8_winF ) ? lrt___WinRegShave8_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave9_winC = DEFINED( lrt___WinRegShave9_winC ) ? lrt___WinRegShave9_winC : ABSOLUTE(0x70128000);    /* Default absolute address  */
  lrt___WinRegShave9_winD = DEFINED( lrt___WinRegShave9_winD ) ? lrt___WinRegShave9_winD : ABSOLUTE(0x70120000);    /* Default absolute address  */
  lrt___WinRegShave9_winE = DEFINED( lrt___WinRegShave9_winE ) ? lrt___WinRegShave9_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave9_winF = DEFINED( lrt___WinRegShave9_winF ) ? lrt___WinRegShave9_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave10_winC = DEFINED(lrt___WinRegShave10_winC ) ?lrt___WinRegShave10_winC : ABSOLUTE(0x70148000);    /* Default absolute address  */
  lrt___WinRegShave10_winD = DEFINED(lrt___WinRegShave10_winD ) ?lrt___WinRegShave10_winD : ABSOLUTE(0x70140000);    /* Default absolute address  */
  lrt___WinRegShave10_winE = DEFINED(lrt___WinRegShave10_winE ) ?lrt___WinRegShave10_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave10_winF = DEFINED(lrt___WinRegShave10_winF ) ?lrt___WinRegShave10_winF : ABSOLUTE(0x00000000);    /* not used                  */
										    
  lrt___WinRegShave11_winC = DEFINED(lrt___WinRegShave11_winC ) ?lrt___WinRegShave11_winC : ABSOLUTE(0x70168000);    /* Default absolute address  */
  lrt___WinRegShave11_winD = DEFINED(lrt___WinRegShave11_winD ) ?lrt___WinRegShave11_winD : ABSOLUTE(0x70160000);    /* Default absolute address  */
  lrt___WinRegShave11_winE = DEFINED(lrt___WinRegShave11_winE ) ?lrt___WinRegShave11_winE : ABSOLUTE(0x00000000);    /* not used                  */
  lrt___WinRegShave11_winF = DEFINED(lrt___WinRegShave11_winF ) ?lrt___WinRegShave11_winF : ABSOLUTE(0x00000000);    /* not used                  */


  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_ranges   0 : { *(.debug_ranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /*Keep the moviAsm assembly debug information*/
  /*Need to take it in as pattern because our moviAsm will prefix the section*/
  .debug_asmline   0 : { *(*.debug_asmline*) }
  /*Keep rel only as debug info*/
  .rel   0 : { *(.rel*) }

  /DISCARD/ : { *(.stab .stabstr .comment) }
  . = 0;
}

SECTIONS {
    /* 
     * locate the special sections in correct place  
     * These sections are required to ensure Myriad is initialized correctly and must be loaded first   
     */	
    .l2.mode    0x20FD0000 : { KEEP(*(.l2.mode)) }           /* L2 cache mode                */
    .cmx.ctrl   0x20FC0020 : { KEEP(*(.cmx.ctrl)) }          /* CMX default layout           */
}

SECTIONS {
    . = 0;
    .stuff : { *(*) }
     ASSERT ( . == 0, ".stuff section caught data! This means your application has sections placed nowhere which may be fatal!" )
}
