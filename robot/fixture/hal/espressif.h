#ifndef __ESPRESSIF_H
#define __ESPRESSIF_H

enum ESP_PROGRAM_ERROR {
  ESP_ERROR_NONE,
  ESP_ERROR_NO_COMMUNICATION,
  ESP_ERROR_BLOCK_FAILED
};

void InitEspressif(void);
ESP_PROGRAM_ERROR ProgramEspressif(void);

#endif
