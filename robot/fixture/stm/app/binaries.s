  ; Factory binaries  
  AREA    ER_BINARIES, DATA, READONLY

// Uncomment this line to build a mini-version without binaries (to speed debugging)
//#define INCBIN ;

  ALIGN
  EXPORT g_CubeTest
  EXPORT g_CubeTestEnd
g_CubeTest
  INCBIN releases\\cubetest.bin
g_CubeTestEnd

  ALIGN
  EXPORT g_CubeStub
  EXPORT g_CubeStubEnd
g_CubeStub
  INCBIN releases\\cubeotp.bin
g_CubeStubEnd

  ALIGN
  EXPORT g_CubeStubFcc
  EXPORT g_CubeStubFccEnd
g_CubeStubFcc
  INCBIN releases\\cubeotpfcc.bin
g_CubeStubFccEnd

  ;DEBUG -- include raw cube bootloader bin for ram-load testing
  ALIGN
  EXPORT g_CubeBoot
  EXPORT g_CubeBootEnd
g_CubeBoot
  //INCBIN releases\\cubeboot.bin
g_CubeBootEnd

  ALIGN
  EXPORT g_BodyTest
  EXPORT g_BodyTestEnd
g_BodyTest
  INCBIN releases\\systest.bin
g_BodyTestEnd

  ALIGN
  EXPORT g_BodyBoot
  EXPORT g_BodyBootEnd
g_BodyBoot
  INCBIN releases\\sysboot.bin
g_BodyBootEnd

  ALIGN
  EXPORT g_Body
  EXPORT g_BodyEnd
g_Body
  INCBIN releases\\syscon_raw.bin
g_BodyEnd

  ALIGN
  EXPORT g_canary
#ifndef INCBIN
g_canary    DCD     0xcab00d1e
#else
g_canary    DCD     0xffffffff
#endif
  
  END
