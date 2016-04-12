#ifndef __UART_H
#define __UART_H

namespace Anki {
	namespace Cozmo {
		namespace HAL {
			namespace UART {
				void init(void);
				void shutdown(void);
				
				void flush(void);
				void read(void* data, int length);
				void write(const void* data, int length);
			}
		}
	}
}

#endif
