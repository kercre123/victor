#include <stdint.h>

#ifndef __UART_H
#define __UART_H

namespace Anki {
	namespace Cozmo {
		namespace HAL {
			namespace UART {
				void init(void);
				void shutdown(void);
				
				bool rx_avail(void);
				uint16_t readWord(void);
				uint8_t readByte(void);
				void writeWord(uint16_t);

				void flush(void);
				void read(void* data, int length);
				void write(const void* data, int length);
			}
		}
	}
}

#endif
