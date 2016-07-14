  ; Factory binaries  
  AREA    ER_BINARIES, DATA, READONLY

// Uncomment this line to build a mini-version without binaries (to speed debugging)
//#define INCBIN ;
    
  ; Note:  Cube and charger share the same binary
  ALIGN
  EXPORT g_Cube
  EXPORT g_CubeEnd
g_Cube
#ifdef FCC
  INCBIN releases\\xsfcc.bin
#else
  INCBIN releases\\xsboot.bin
#endif
g_CubeEnd

  ALIGN
  EXPORT g_Body
  EXPORT g_BodyEnd
g_Body
  ; FCC support is built into factory syscon
  INCBIN releases\\syscon.bin
g_BodyEnd

  ALIGN
  EXPORT g_BodyBoot
  EXPORT g_BodyBootEnd
g_BodyBoot
  INCBIN releases\\sys_boot.bin
g_BodyBootEnd

  ALIGN
  EXPORT g_BodyBLE
  EXPORT g_BodyBLEEnd
g_BodyBLE
  INCBIN releases\\s110_softdevice.bin
g_BodyBLEEnd

  ALIGN
  EXPORT g_stubBody
  EXPORT g_stubBodyEnd
g_stubBody
  INCBIN releases\\nrf51_stub.bin
g_stubBodyEnd


  ALIGN
  EXPORT g_K02
  EXPORT g_K02End
g_K02
#ifdef FCC
  INCBIN releases\\robot.fcc.bin
#else
  INCBIN releases\\robot.bin
#endif
g_K02End

  ALIGN
  EXPORT g_K02Boot
  EXPORT g_K02BootEnd
g_K02Boot
  INCBIN releases\\robot_boot.bin
g_K02BootEnd

  ALIGN
  EXPORT g_stubK02
  EXPORT g_stubK02End
g_stubK02
  INCBIN releases\\k02_stub.bin
g_stubK02End



  ALIGN
  EXPORT g_EspBlank
  EXPORT g_EspBlankEnd
g_EspBlank
  INCBIN releases\\esp.blank.bin
g_EspBlankEnd

  ALIGN
  EXPORT g_EspUser
  EXPORT g_EspUserEnd
g_EspUser

#ifdef FCC
  INCBIN releases\\esp.fcc.bin
g_EspUserEnd

#else
  INCBIN releases\\esp.factory.bin
g_EspUserEnd

  ALIGN
  EXPORT g_EspBoot
  EXPORT g_EspBootEnd
g_EspBoot
  INCBIN releases\\esp.boot.bin
g_EspBootEnd

  ALIGN
  EXPORT g_EspInit
  EXPORT g_EspInitEnd
g_EspInit
  INCBIN releases\\esp.init.bin
g_EspInitEnd

  ALIGN
  EXPORT g_EspSafe
  EXPORT g_EspSafeEnd
g_EspSafe
  INCBIN releases\\esp.safe.bin
g_EspSafeEnd


  ALIGN
  EXPORT g_Radio
  EXPORT g_RadioEnd
g_Radio
  INCBIN releases\\radio.bin
g_RadioEnd
#endif

  ALIGN
  EXPORT g_canary
#ifndef INCBIN
g_canary    DCD     0xcab00d1e
#else
g_canary    DCD     0xffffffff
#endif
  
  END
