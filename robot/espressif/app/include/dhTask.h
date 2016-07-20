#ifndef __ESP_DIFFIE_H
#define __ESP_DIFFIE_H

namespace DiffieHellman {
	bool Init();
	void SetLocal(const uint8_t* local);
	void SetRemote(const uint8_t* remote);
	void Update();
}

#endif
