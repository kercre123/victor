/**
****************************************************************************************
*
* @file dht11.c
*
* @brief DHT11 Sensor driver. Reads temperature and humidity.
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

#include "dht11.h"
#include "systick.h"

const uint32_t MAX_DURATION = 100000; // response should not take more that 100 ms
const int BYTES = 4;
uint8_t bytes[BYTES];
dht11_status_t dht11_status = DHT11_RESET;
GPIO_PIN gpio_pin;
GPIO_PORT gpio_port;
IRQn_Type gpio_irq = (IRQn_Type)0;
uint32_t irq_time;

/**
 ****************************************************************************************
 * @brief Saves the falling time of GPIO_pin indicating a response from the sensor
 *
 * @param[in] void
 *
 * @return void
 ****************************************************************************************
 */
void gpio_isr(void)
{
    irq_time = systick_value();
}

/**
 ****************************************************************************************
 * @brief Restores SysTick and GPIO configuration and returns success or failure
 *
 * @param[in] status DHT11_READY or DHT11_FAIL
 *
 * @return status
 ****************************************************************************************
 */
dht11_status_t dht11_reset(dht11_status_t status)
{
    systick_stop();

    // The Single-Bus voltage will be pulled up by the resistor
    // to set it back to the free status.
    GPIO_ConfigurePin(gpio_port, gpio_pin, OUTPUT, PID_GPIO, true);
    
    dht11_status = DHT11_RESET;
    return status;
}

/**
 ****************************************************************************************
 * @brief Receives a bit
 *
 * @param[in] void
 *
 * @return the bit received or -1 if reception timed out
 ****************************************************************************************
 */
int8_t dht11_bit_get(void)
{
    uint32_t start_time = irq_time;
    
    while (irq_time == start_time)
        if (start_time-systick_value() > 200) // 200 usec time-out
            return -1;
    return (start_time-irq_time < 100) ? 0 : 1;
}


/**
 ****************************************************************************************
 * @brief Receives a byte
 *
 * @param[in] byte where to write the byte received
 *
 * @return TRUE if reception succeeded, FALSE if not
 ****************************************************************************************
 */ 
bool dht11_byte_get(uint8_t *byte)
{
    if (byte == NULL)
        return false;

    *byte = 0;

    int8_t bit;

    // the sensor sends higher data bit first
    for (int order = 7; order >= 0; order--)
    {
        if ((bit=dht11_bit_get()) < 0)
            return false;
        *byte |= (bit << order);
    }
    return true;
}

/**
 ****************************************************************************************
 * @brief After enough time for the DHT’s detection of MCU's signal has passed,
 *        prepare for receiving the DHT's response on GPIO_pin
 *
 * @param[in] void
 *
 * @return void
 ****************************************************************************************
 */
void systick_isr(void)
{
    systick_stop();

    // pull up voltage and wait 20-40us for DHT’s response
    GPIO_SetActive(gpio_port, gpio_pin);
    systick_wait(20);
    GPIO_ConfigurePin(gpio_port, gpio_pin, INPUT_PULLUP, PID_GPIO, false);

    systick_start(MAX_DURATION, false);
    irq_time = MAX_DURATION - 1;

    // When DHT is sending data to MCU,
    // every bit of data begins with the 50us low-voltage-level
    GPIO_RegisterCallback(gpio_irq, gpio_isr);
    GPIO_EnableIRQ(gpio_port, gpio_pin, gpio_irq, true, true, 0);
	NVIC_EnableIRQ(gpio_irq);
    dht11_status = DHT11_READY;
}

/**
 ****************************************************************************************
 * @brief Unless already started, select the GPIO port, pin and IRQ for communicating
 *        with the sensor and start a new request for data
 *
 * @param[in] port the GPIO port where the pin of the sensor is connected
 * @param[in] pin the GPIO pin where the pin of the sensor is connected
 * @param[in] irq the GPIO irq raised when the sensor responds
 *
 * @return void
 ****************************************************************************************
 */
void dht11_init(GPIO_PORT port, GPIO_PIN pin, IRQn_Type irq)
{
    if (dht11_status != DHT11_BUSY)
    {
        gpio_port = port;
        gpio_pin = pin;
        gpio_irq = irq;
        dht11_status = DHT11_BUSY;
        // set Data Single-bus voltage level from high to low
        // for at least 18ms to ensure DHT’s detection of MCU's signal
        GPIO_SetInactive(port, pin);
        systick_register_callback(systick_isr);
        systick_usec_units(true);
        systick_start(18000, true);
    }
}

/**
 ****************************************************************************************
 * @brief Reads temperature and humidity
 *
 * @param[in] temperature   where to return the temperature
 * @param[in] humidity      where to return the humidity
 *
 * @return NULL, if temperature or humidity is NULL,
 *         DHT11_INIT_FIRST, if DHT11_Init() has not been called yet,
 *         DHT11_BUSY, if the MCU is still sending the request to the sensor,
 *         DHT11_FAIL, if the sensor did not respond as expected,
 *         DHT11_READY, if the sensor responded as expected data is returned
 ****************************************************************************************
 */ 
dht11_status_t dht11_read(uint8_t* temperature, uint8_t* humidity)
{
    if (temperature == NULL || humidity == NULL)
        return DHT11_NULL_PARAM;
    if (gpio_irq < GPIO0_IRQn)
        return DHT11_INIT_FIRST;
    if (dht11_status != DHT11_READY)
    {
        dht11_init(gpio_port, gpio_pin, gpio_irq);
        return DHT11_BUSY;
    }
    // Once DHT sent out the response signal,
    // it pulls up voltage and keeps it for 80us and prepares for data transmission.
    if (dht11_bit_get() < 1)
        return dht11_reset(DHT11_FAIL);

    int checksum_calculated = 0;
    for (int byte = 0; byte < BYTES; byte++)
    {
        if (!dht11_byte_get(bytes + byte))
            return dht11_reset(DHT11_FAIL);
        checksum_calculated += bytes[byte];
    }

    uint8_t checksum_received;
    if (!dht11_byte_get(&checksum_received) || (uint8_t)checksum_calculated != checksum_received)
        return dht11_reset(DHT11_FAIL);

    systick_stop();

    // When the last bit data is transmitted,
    // DHT11 pulls down the voltage level and keeps it for 50us.
    systick_wait(50);
                
    *humidity = bytes[0];
    *temperature = bytes[2];
    return dht11_reset(DHT11_READY);
}
