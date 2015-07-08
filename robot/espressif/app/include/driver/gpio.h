/** @file Espressif GPIO driver interface
 * @author Daniel Casner <daniel@anki.com>
 * Abstractions of the 16 gpio pins on the Espressif.
 */
#ifndef GPIO_APP_H
#define GPIO_APP_H

/// Reference the ROM registers
extern volatile uint32_t PIN_OUT; ///< Write to set the level of all pins at once
extern volatile uint32_t PIN_OUT_SET; ///< Write mask to set pins high, doesn't affect non-masked pins
extern volatile uint32_t PIN_OUT_CLEAR; ///< Write mask to set pins low, doesn't affect non-masked pins
extern volatile uint32_t PIN_DIR; ///< Write to set the direction of all pins at once
extern volatile uint32_t PIN_DIR_OUTPUT; ///< Write mask to set pins as outputs, doesn't affect non-masked pins
extern volatile uint32_t PIN_DIR_INPUT; ///< Write mask to set pins as inputs, doesn't affect non-masked pins
extern volatile uint32_t PIN_IN; ///< Read to access the current state of pins as input
extern volatile uint32_t PIN_0; ///< ???
extern volatile uint32_t PIN_2; ///< ???




#endif
