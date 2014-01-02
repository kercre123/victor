// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator interrupts
// -----------------------------------------------------------------------------

#ifndef __SIPP_IRQ_H__
#define __SIPP_IRQ_H__

#include "moviThreadsUtils.h"

#define SIPP_NIRQ 3

class SippIrq {

public:
	SippIrq (void (*pNotifyCallback)(void *) = 0, void *pObject = 0) :
		pIrqNotify (pNotifyCallback),
		pObj       (pObject) {
			for (int i = 0; i < SIPP_NIRQ; i++) {
				enable[i] = 0;
				status[i] = 0;
			}
	  }

	  ~SippIrq () {
	  }

	  // Interrupt status/enable access methods for SippHW ApbWrite/ApbRead
	  void SetEnable (int id, int val) {
		  enable[id] = val;
	  }
	  int GetEnable (int id) {
		  return enable[id];
	  }
	  int GetStatus (int id) {
		  return status[id] & enable[id];
	  }
	  void ClrStatus (int id, int val) {
		  status_m.Lock();
		  status[id] &= ~val;
		  status_m.Unlock();
	  }

	  // Interrupt status bit set callback passed to filter objects
	  void SetStatusBit (int id, int bit) {
		  status_m.Lock();
		  status[id] |= 1 << bit;
		  status_m.Unlock();

		  // If interrupt is enabled call interrupt notification callback
		  // function (if assigned)
		  if ( (enable[id] & (1 << bit)) && (pIrqNotify != 0) )
			  pIrqNotify(pObj);
	  }


private:
	int status[SIPP_NIRQ];
	int enable[SIPP_NIRQ];

	Mutex status_m;                       // Mutex for access to status

	void (*pIrqNotify)(void *);
	void *pObj;
};

#endif // __SIPP_IRQ_H__
