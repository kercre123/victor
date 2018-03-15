#ifndef __APPLICATION_API_H
#define __APPLICATION_API_H

typedef struct {
	uint8_t version[0x10];
	void (*AppInit)();
	void (*AppDeInit)();
	void (*AppTick)();
	void (*BLE_Recv)(uint8_t, const void*);
	void (*BLE_Send)(uint8_t, const void*);
	uint8_t nonce[16];
	uint8_t app_data[0x27CC];
} ApplicationMap;

#endif
