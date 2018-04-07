/**
 ****************************************************************************************
 *
 * @file ring_buffer.h
 *
 * @brief Ring buffer API.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H


#include <stdint.h>
#include <stdbool.h>


bool buffer_is_empty(void);

bool buffer_is_full(void);

void buffer_put_byte(uint8_t byte);

int buffer_get_byte(uint8_t *byte);

#endif // _RING_BUFFER_H
