/**
****************************************************************************************
*
* @file dht11.h
*
* @brief DHT11 sensor driver header.
*
* Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
*
* <bluetooth.support@diasemi.com> and contributors.
*
****************************************************************************************
*/
#ifndef _DHT11_H
    #define _DHT11_H

    #include "gpio.h"

typedef enum
{
    DHT11_RESET,
    DHT11_BUSY,
    DHT11_FAIL,
    DHT11_READY,
    DHT11_NULL_PARAM,
    DHT11_INIT_FIRST
} dht11_status_t;

void dht11_init(GPIO_PORT port, GPIO_PIN pin, IRQn_Type irq);
dht11_status_t dht11_read(uint8_t* temperature, uint8_t* humidity);

#endif //_DHT11_H

