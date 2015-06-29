#include "lights.h"
#include "nrf.h"
#include "nrf_gpio.h"

// XXX: Each channel is binary. No blending is currently supported

#define RED_COLOR_MASK 0xFF << 16
#define GREEN_COLOR_MASK 0xFF << 8
#define BLUE_COLOR_MASK 0xFF

#include "hardware.h"

#include "hal/portable.h"

// Define charlie wiring here:
charliePlex_s RGBLightPins[numCharlieChannels] =
{
	// anode, cath_red, cath_gree, cath_blue
	{PIN_LED1, PIN_LED2, PIN_LED3, PIN_LED4},
	{PIN_LED2, PIN_LED1, PIN_LED3, PIN_LED4},
	{PIN_LED3, PIN_LED1, PIN_LED2, PIN_LED4},
	{PIN_LED4, PIN_LED1, PIN_LED2, PIN_LED3}
};

	// Start all pins as input
void Lights::init()
{
	nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
}
	
// Configure pins for a particular channel
static void Lights::setPins(charliePlex_s pins, uint32_t color)
{
	Lights::init();
	
	// Set anode to output high
	nrf_gpio_pin_set(pins.anode);
	nrf_gpio_cfg_output(pins.anode);	
	// RED
	if (!!(color & RED_COLOR_MASK))
	{
		nrf_gpio_pin_clear(pins.cath_red);
		nrf_gpio_cfg_output(pins.cath_red);	
	}
	// GREEN
	if (!!(color & GREEN_COLOR_MASK))
	{
		nrf_gpio_pin_clear(pins.cath_green);
		nrf_gpio_cfg_output(pins.cath_green);	
	}
	// BLUE
	if (!!(color & BLUE_COLOR_MASK))
	{
		nrf_gpio_pin_clear(pins.cath_blue);
		nrf_gpio_cfg_output(pins.cath_blue);	
	}
}

// Manage lights. Each call increments the state machine
void Lights::manage(volatile uint32_t *colors)
{
	static char charlieChannel = RGB1;	

	// Set lights for current charlie channel
	Lights::setPins(RGBLightPins[charlieChannel], colors[charlieChannel]);
	
	// Get next charlie channel
	charlieChannel++;	
	if(charlieChannel == numCharlieChannels)
		charlieChannel = RGB1;

}
