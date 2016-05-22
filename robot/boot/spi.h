#ifndef __SPI_H
#define __SPI_H

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
			namespace SPI
			{
				void espInit(void);
				void init(void);
        void disable(void);
				void sync(void);
				
				uint16_t readWord(void);
			}
		}
	}
}

#endif
