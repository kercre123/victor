#ifndef __SIDELOAD_H
#define __SIDELOAD_H

static const int SIDELOAD_SPACE_START = 0x20000000;
static const int SIDELOAD_SPACE_LENGTH = 0x2000;

typedef bool (*sideload_call)(void);

static const sideload_call* const SIDELOAD_DATABASE = (sideload_call*) SIDELOAD_SPACE_START;

#endif
