#ifndef __ESPRESSIF_H
#define __ESPRESSIF_H

#include <stdint.h>

void InitEspressif(void);
void DeinitEspressif(void);

void ProgramEspressif(void);
bool ESPFlashLoad(uint32_t address, int length, const uint8_t *data);

#endif
