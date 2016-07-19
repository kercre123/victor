#ifndef __ESP_DIFFIE_H
#define __ESP_DIFFIE_H

namespace DiffieHellman {
	void Init(uint8_t local[], uint8_t remote[]);
	void Update();
}

#endif
