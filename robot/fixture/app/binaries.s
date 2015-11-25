  ; Factory binaries
  
  AREA    ER_BINARIES, DATA, READONLY

  ALIGN

  ; Note:  Cube and charger share the same binary
  EXPORT g_Cube
  EXPORT g_CubeEnd
g_Cube
  INCBIN releases\\cube.bin
g_CubeEnd


  ALIGN

  EXPORT g_Body
  EXPORT g_BodyEnd
g_Body
  INCBIN releases\\syscon.bin
g_BodyEnd

  ALIGN

  EXPORT g_BodyBoot
  EXPORT g_BodyBootEnd
g_BodyBoot
  INCBIN releases\\sys_boot.bin
g_BodyBootEnd


  ALIGN

  EXPORT g_K02
  EXPORT g_K02End
g_K02
  INCBIN releases\\robot.bin
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
  ;INCBIN releases\\esp.user.bin
  INCBIN releases\\esp.boot.bin
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

  EXPORT g_canary
g_canary    DCD     0xcab00d1e

  END
