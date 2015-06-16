/** @file Upgrade / flash controller
 * This module is responsible for reflashing the Espressif
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __upgrade_controller_h
#define __upgrade_controller_h

/// Initalize the upgrade controller
int8_t upgradeControllerInit(void);

/// Port the espressif will listen on for upgrade commands
#define UPGRADE_CONTROLLER_COMMAND_PORT 8580 ///< = "UP"

/// Header for upgrade command packets
#define UPGRADE_COMMAND_PREFIX "robotsRising"
/// Length of header
#define UPGRADE_COMMAND_PREFIX_LENGTH 12

/// An impossible flash address to invalidate the state
#define INVALID_FLASH_ADDRESS (0xFFFFffff)

/// Upgrade command controll flags
// One hot encoding is used, might mask in the future
typedef enum {
  UPCMD_FLAGS_NONE = 0x00, // No special actions
  UPCMD_WIFI_FW    = 0x01, // Upgrade the wifi (Espressif firmware)
  UPCMD_CTRL_FW    = 0x02, // Upgrade the robot supervisor firmware
  UPCMD_FPGA_FW    = 0x04, // Upgrade the FPGA image
  UPCMD_BODY_FW    = 0x08, // Upgrade the body board firmware
  UPCMD_ASK_WHICH  = 0x80, // Ask the espressif which firmware it wants
} UpgradeCommandFlags;

/// Parameters for an firmware upgrade command
typedef struct _UpradeCommandParameters
{
  char PREFIX[UPGRADE_COMMAND_PREFIX_LENGTH]; ///< Communication header
  uint32_t flashAddress;                      ///< Where to put the data
  uint32_t size;                              ///< The number of bytes to expect and write
  uint8_t  flags;                             ///< Assorted flags for the upgrade
} UpgradeCommandParameters;

#endif
