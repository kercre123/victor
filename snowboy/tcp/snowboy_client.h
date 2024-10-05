// snowboy_client.h

#ifndef SNOWBOY_CLIENT_H
#define SNOWBOY_CLIENT_H

#include <stdint.h>

int snowboy_init(void);
int snowboy_process(const char* data, uint32_t data_length);
void snowboy_close(void);

#endif // SNOWBOY_CLIENT_H
