  ; Factory binaries  
  ;AREA    ER_BINARIES, DATA, READONLY
  AREA    ER_IROM1, DATA, READONLY
  
  ;--------------------------
  ;         CubeBoot
  ;--------------------------
  ALIGN
  EXPORT g_CubeBoot
  EXPORT g_CubeBootEnd
g_CubeBoot
  INCBIN releases\\cubefcc.bin
g_CubeBootEnd
  
  END
