/**
 ****************************************************************************************
 *
 * @file ring_buffer.c
 *
 * @brief Ring buffer implementation.
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

#include <stdint.h>
#include <stdbool.h>

static const unsigned int BUFFER_SIZE = 128;

static uint8_t buffer[BUFFER_SIZE];
static unsigned int buffer_count = 0;
static unsigned int buffer_head = 0;
static unsigned int buffer_tail = 0;

bool buffer_is_empty(void)
{
    return buffer_count == 0;
}

bool buffer_is_full(void)
{
    return buffer_count == BUFFER_SIZE;
}

void buffer_put_byte(uint8_t byte)
{
    if(!buffer_is_full())
    {
        buffer[buffer_head] = byte;
        buffer_head = (buffer_head + 1) % BUFFER_SIZE;
        buffer_count += 1;
    }
}

int buffer_get_byte(uint8_t *byte)
{
    int status = -1; // failure
    
    if(!buffer_is_empty())
    {
        *byte = buffer[buffer_tail];
        buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
        buffer_count -= 1;
        
        status = 0; // success
    }
    return status;
}
