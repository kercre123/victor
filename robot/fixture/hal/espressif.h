#ifndef __ESPRESSIF_H
#define __ESPRESSIF_H

#include <stdint.h>

void InitEspressif(void);
void DeinitEspressif(void);

void ProgramEspressif(int serial);
bool ESPFlashLoad(uint32_t address, int length, const uint8_t *data, bool dat_static_fill ); //dat_static_fill==true, uses single u8 value data[0] as memory fill value.

#endif
