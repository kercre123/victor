/********************************************************************************************************************
   ------------------------------------------------------------------------------------------------------------------
    Sections included from shave_cmx.lds.h
    - This block defines the placement of the code/data for all shaves and also creates symbols for CMX slices
   ------------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
  
SECTIONS {
   /* Declaring symbols for Shave Slices */
  . = 0x70000000;  __CMX_SLICE0_START = .;
  . = 0x70020000;  __CMX_SLICE1_START = .;
  . = 0x70040000;  __CMX_SLICE2_START = .; 
  . = 0x70060000;  __CMX_SLICE3_START = .; 
  . = 0x70080000;  __CMX_SLICE4_START = .; 
  . = 0x700A0000;  __CMX_SLICE5_START = .; 
  . = 0x700C0000;  __CMX_SLICE6_START = .; 
  . = 0x700E0000;  __CMX_SLICE7_START = .; 
  . = 0x70100000;  __CMX_SLICE8_START = .; 
  . = 0x70120000;  __CMX_SLICE9_START = .; 
  . = 0x70140000;  __CMX_SLICE10_START = .; 
  . = 0x70160000;  __CMX_SLICE11_START = .; 
  . = 0x70180000;  __CMX_SLICE12_START = .; 
  . = 0x701A0000;  __CMX_SLICE13_START = .; 
  . = 0x701C0000;  __CMX_SLICE14_START = .; 
  . = 0x701E0000;  __CMX_SLICE15_START = .; 
  . = 0x70200000;  __CMX_END = .;
}
        
SECTIONS {
  /* Setting up location of code and data for all Shaves */

  . = MV_SVE0_TEXT_BEGIN;
  S.shv0.cmx.text : { 
        *(.shv0.S.text*)
        *(.shv0.S.__TEXT__sect)
        *(.shv0.S.__MAIN__sect) 
        *(.shv0.S.init*)
  }
  .cmx.data_slice0 : { 
        *(.slice0.data) 
  }
  
  . = MV_SVE0_DATA_BEGIN;
  S.shv0.cmx.data : { 
        *(.shv0.S.data*)
        *(.shv0.S.rodata*)
        *(.shv0.S.__DATA__sect*)
        *(.shv0.S.__STACK__sect*)
        *(.shv0.S.__static_data*)
        *(.shv0.S.__HEAP__sect*)
        *(.shv0.S.__T__*)
  }
  
  . = MV_SVE1_TEXT_BEGIN;
  
  S.shv1.cmx.text : { 
        *(.shv1.S.text*)
        *(.shv1.S.__TEXT__sect)
        *(.shv1.S.__MAIN__sect)
        *(.shv1.S.init*)
  }
  . = MV_SVE1_DATA_BEGIN;
  S.shv1.cmx.data : { 
        *(.shv1.S.data*)
        *(.shv1.S.rodata*)
        *(.shv1.S.__DATA__sect*)
        *(.shv1.S.__STACK__sect*)
        *(.shv1.S.__static_data*)
        *(.shv1.S.__HEAP__sect*)
        *(.shv1.S.__T__*)
  }  
  
  . = MV_SVE2_TEXT_BEGIN;
  S.shv2.cmx.text : { 
        *(.shv2.S.text*)
        *(.shv2.S.__TEXT__sect)
        *(.shv2.S.__MAIN__sect)
        *(.shv2.S.init*)
  }
  . = MV_SVE2_DATA_BEGIN;
  S.shv2.cmx.data : { 
        *(.shv2.S.data*)
        *(.shv2.S.rodata*)
        *(.shv2.S.__DATA__sect*)
        *(.shv2.S.__STACK__sect*)
        *(.shv2.S.__static_data*)
        *(.shv2.S.__HEAP__sect*)
        *(.shv2.S.__T__*)
  }
  
  . = MV_SVE3_TEXT_BEGIN;
  S.shv3.cmx.text : { 
        *(.shv3.S.text*)
        *(.shv3.S.__TEXT__sect)
        *(.shv3.S.__MAIN__sect)
        *(.shv3.S.init*)
  }
  . = MV_SVE3_DATA_BEGIN;
  S.shv3.cmx.data : { 
        *(.shv3.S.data*)
        *(.shv3.S.rodata*)
        *(.shv3.S.__DATA__sect*)
        *(.shv3.S.__STACK__sect*)
        *(.shv3.S.__static_data*)
        *(.shv3.S.__HEAP__sect*)
        *(.shv3.S.__T__*)
  }  
  
  . = MV_SVE4_TEXT_BEGIN;
  S.shv4.cmx.text : { 
        *(.shv4.S.text*)
        *(.shv4.S.__TEXT__sect)
        *(.shv4.S.__MAIN__sect)
        *(.shv4.S.init*)
  }
  . = MV_SVE4_DATA_BEGIN;
  S.shv4.cmx.data : { 
        *(.shv4.S.data*)
        *(.shv4.S.rodata*)
        *(.shv4.S.__DATA__sect*)
        *(.shv4.S.__STACK__sect*)
        *(.shv4.S.__static_data*)
        *(.shv4.S.__HEAP__sect*)
        *(.shv4.S.__T__*)
  }

  . = MV_SVE5_TEXT_BEGIN;
  S.shv5.cmx.text : { 
        *(.shv5.S.text*)
        *(.shv5.S.__TEXT__sect)
        *(.shv5.S.__MAIN__sect)
        *(.shv5.S.init*)
  }
  . = MV_SVE5_DATA_BEGIN;
  S.shv5.cmx.data : { 
        *(.shv5.S.data*)
        *(.shv5.S.rodata*)
        *(.shv5.S.__DATA__sect*)
        *(.shv5.S.__STACK__sect*)
        *(.shv5.S.__static_data*)
        *(.shv5.S.__HEAP__sect*)
        *(.shv5.S.__T__*)
  }
  
  . = MV_SVE6_TEXT_BEGIN;
  S.shv6.cmx.text : { 
        *(.shv6.S.text*)
        *(.shv6.S.__TEXT__sect)
        *(.shv6.S.__MAIN__sect)
        *(.shv6.S.init*)
  }
  . = MV_SVE6_DATA_BEGIN;
  S.shv6.cmx.data : { 
        *(.shv6.S.data*)
        *(.shv6.S.rodata*)
        *(.shv6.S.__DATA__sect*)
        *(.shv6.S.__STACK__sect*)
        *(.shv6.S.__static_data*)
        *(.shv6.S.__HEAP__sect*)
        *(.shv6.S.__T__*)
  }  
  
  . = MV_SVE7_TEXT_BEGIN;
  S.shv7.cmx.text : { 
        *(.shv7.S.text*)
        *(.shv7.S.__TEXT__sect)
        *(.shv7.S.__MAIN__sect)
        *(.shv7.S.init*)
  }
  . = MV_SVE7_DATA_BEGIN;
  S.shv7.cmx.data : { 
        *(.shv7.S.data*)
        *(.shv7.S.rodata*)
        *(.shv7.S.__DATA__sect*)
        *(.shv7.S.__STACK__sect*)
        *(.shv7.S.__static_data*)
        *(.shv7.S.__HEAP__sect*)
        *(.shv7.S.__T__*)
  }
  
  . = MV_SVE8_TEXT_BEGIN;
  S.shv8.cmx.text : { 
        *(.shv8.S.text*)
        *(.shv8.S.__TEXT__sect)
        *(.shv8.S.__MAIN__sect)
        *(.shv8.S.init*)
  }
  . = MV_SVE8_DATA_BEGIN;
  S.shv8.cmx.data : { 
        *(.shv8.S.data*)
        *(.shv8.S.rodata*)
        *(.shv8.S.__DATA__sect*)
        *(.shv8.S.__STACK__sect*)
        *(.shv8.S.__static_data*)
        *(.shv8.S.__HEAP__sect*)
        *(.shv8.S.__T__*)
  }
  
  . = MV_SVE9_TEXT_BEGIN;
  S.shv9.cmx.text : { 
        *(.shv9.S.text*)
        *(.shv9.S.__TEXT__sect)
        *(.shv9.S.__MAIN__sect)
        *(.shv9.S.init*)
  }
  . = MV_SVE9_DATA_BEGIN;
  S.shv9.cmx.data : { 
        *(.shv9.S.data*)
        *(.shv9.S.rodata*)
        *(.shv9.S.__DATA__sect*)
        *(.shv9.S.__STACK__sect*)
        *(.shv9.S.__static_data*)
        *(.shv9.S.__HEAP__sect*)
        *(.shv9.S.__T__*)
  }
  
  . = MV_SVE10_TEXT_BEGIN;
  S.shv10.cmx.text : { 
        *(.shv10.S.text*)
        *(.shv10.S.__TEXT__sect)
        *(.shv10.S.__MAIN__sect)
        *(.shv10.S.init*)
  }
  . = MV_SVE10_DATA_BEGIN;
  S.shv10.cmx.data : { 
        *(.shv10.S.data*)
        *(.shv10.S.rodata*)
        *(.shv10.S.__DATA__sect*)
        *(.shv10.S.__STACK__sect*)
        *(.shv10.S.__static_data*)
        *(.shv10.S.__HEAP__sect*)
        *(.shv10.S.__T__*)
  }
  
  . = MV_SVE11_TEXT_BEGIN;
  S.shv11.cmx.text : { 
        *(.shv11.S.text*)
        *(.shv11.S.__TEXT__sect)
        *(.shv11.S.__MAIN__sect)
        *(.shv11.S.init*)
  }
  . = MV_SVE11_DATA_BEGIN;
  S.shv11.cmx.data : { 
        *(.shv11.S.data*)
        *(.shv11.S.rodata*)
        *(.shv11.S.__DATA__sect*)
        *(.shv11.S.__STACK__sect*)
        *(.shv11.S.__static_data*)
        *(.shv11.S.__HEAP__sect*)
        *(.shv11.S.__T__*)
  }

  /* TODO: The following line needs to be removed */
  __NEXT_FREE_CMX_SLICE = ALIGN(4);
}

/*The following are empty sections but we just need to make sure their symbols get pulled in*/
SECTIONS {

  . = 0x1D000000;
  S.shvdynamic.text : {
        *(.dyn.text*)
  }

  . = 0x1C000000;
  S.shvdynamic.data : {
        *(.dyn.data*)
  }

}

