#ifndef __IRQ_H
#define __IRQ_H

typedef void (*IrqCallback)(void);
static IrqCallback* IRQ_Vectors = (IrqCallback*) 0x20000040;

#endif
