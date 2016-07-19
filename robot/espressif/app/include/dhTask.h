#ifndef __ESP_DIFFIE_H
#define __ESP_DIFFIE_H

namespace DiffieHellman {
	bool Init();
	void Start(const uint8_t* local, const uint8_t* remote);
	void Update();
}

#endif
