/** @file Retrieves factory data written by fixtures into Espressif flash
 * @author Daniel Casner <daniel@anki.com>
 */
 
#ifndef __ESPRESSIF_DRIVER_FACTORY_DATA_H_
#define __ESPRESSIF_DRIVER_FACTORY_DATA_H_

/// Initalize factory data driver
// Copies some data into RAM for reliable fast access
void factoryDataInit(void);

/// Returns the robot's serial number or 0xFFFFffff if not set
uint32_t getSerialNumber(void);

/// Returns the SSID for the robot based on serial number
uint32_t getSSIDNumber(void);

/// Returns the version of the factory firmware on the robot
uint32_t getFactoryFirmwareVersion(void);

/// Returns the robot's model number or 0xffff if not set
uint16_t getModelNumber(void);

/// Retrieves the random seed populated in flash by the factory fixture
bool getFactoryRandomSeed(uint32_t* dest, const int len);

/// Retrieves the random string populated in flash by the factory fixture
// dest_sz <= DIGITDASH_WIFI_PSK_LEN
bool getFactoryGeneratedPsk(char dest[32], const int dest_sz);

#endif
