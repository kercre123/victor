#include "lights.h"
#include "nrf.h"
#include "nrf_gpio.h"

// XXX: Each channel is binary. No blending is currently supported

#define RED_COLOR_MASK 0xFF << 16
#define GREEN_COLOR_MASK 0xFF << 8
#define BLUE_COLOR_MASK 0xFF

const u8 PIN_LED1	= 18;
const u8 PIN_LED2	= 19;
const u8 PIN_LED3	= 9;
const u8 PIN_LED4	= 29;

struct charliePlex_s
{
	u8 anode;
	u8 cath_red;
	u8 cath_green;
	u8 cath_blue;
};

enum charlieChannels_e
{
	RGB1,
	RGB2,
	RGB3,
	RGB4,
	numCharlieChannels
};

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
void InitLights()
{
	nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(PIN_LED2, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(PIN_LED3, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(PIN_LED4, NRF_GPIO_PIN_NOPULL);
}
	
// Configure pins for a particular channel
static void SetLightPins(charliePlex_s pins, u32 color)
{
	InitLights();
	
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
void ManageLights(volatile uint32_t *colors)
{
	static char charlieChannel = RGB1;	

	// Set lights for current charlie channel
	SetLightPins(RGBLightPins[charlieChannel], colors[charlieChannel]);
	
	// Get next charlie channel
	if(charlieChannel == numCharlieChannels)
		charlieChannel = RGB1;
	else
		charlieChannel++;	
}
