#include <stdint.h>
#include "user_config.h"
#include "driver/uart.h"

/////////////////////////////////////////////////////////////
// Overrides for libnet80211.a



/*
 Fix error where ieee80211_getmgtframe allocates an "esf_buf", 
   which is a structure which contains pointers to other allocations,
   but esf_buf_alloc does not do error checking on the sub-alloc results.
*/
void *__real_ieee80211_getmgtframe(uint8_t **frm, int headroom, int pktlen);


void *__wrap_ieee80211_getmgtframe(uint8_t **frm, int headroom, int pktlen)
{
   // experiments show pktlen requests > 16 allocate 0x140 bytes total.
   // So, ensure we have enough heap before calling.
   
   // I verified real func only modifies *frm on non-null return,
   //  so it's ok to return NULL if alloc would fail.
   
   if (pktlen> 16 && xPortGetFreeHeapSize() < 0x140) {
      os_put_char('m');os_put_char('0');
      return NULL;
   }
   return  __real_ieee80211_getmgtframe(frm, headroom, pktlen);
}
/////////////////////////////////////////////////////////////

