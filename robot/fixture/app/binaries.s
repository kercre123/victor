  ; Factory binaries
  
  AREA    ER_ROM1, DATA, READONLY

  EXPORT g_EspBlank
  EXPORT g_EspBlankEnd
  ;EXPORT g_EspUser
  ;EXPORT g_EspUserEnd
  EXPORT g_EspBoot
  EXPORT g_EspBootEnd
  EXPORT g_EspInit
  EXPORT g_EspInitEnd

g_EspBlank
  INCBIN ..\\bin\\esp.blank.bin
g_EspBlankEnd
g_EspUser
  ;INCBIN ..\\bin\\esp.user.bin
g_EspUserEnd
g_EspBoot
  INCBIN ..\\bin\\esp.boot.bin
g_EspBootEnd
g_EspInit
  INCBIN ..\\bin\\esp.init.bin
g_EspInitEnd

  END
