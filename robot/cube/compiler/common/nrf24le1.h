#ifndef NRF24LE1_H_
#define NRF24LE1_H_

#ifdef __ICC8051__
#include <ionRF24LE1.h>
#include <bit24le1.h>
#endif /*ICC8051*/

#ifdef __C51__
#include <Nordic\reg24le1.h>
#endif /*__C51__*/

#include "isrdef24le1.h"
#include "memdefs.h"

#endif /*NRF24LE1_H_*/
