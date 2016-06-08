#ifdef TARGET_ESPRESSIF
extern "C" {
    #include "ets_sys.h"
    #include "osapi.h"
}

#define memcpy os_memcpy
#define memset os_memset
#define memcmp os_memcmp

#endif
