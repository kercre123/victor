  ; Factory binaries
  AREA    ER_IROM1, DATA, READONLY

  ; Ran out of space in the ER_BINARIES region.
  ; stick a few bins in code region.
  
  ALIGN
  EXPORT g_stubBody
  EXPORT g_stubBodyEnd
g_stubBody
  INCBIN releases\\nrf51_stub.bin
g_stubBodyEnd
  
  ALIGN
  EXPORT g_stubK02
  EXPORT g_stubK02End
g_stubK02
  INCBIN releases\\k02_stub.bin
g_stubK02End

  END
