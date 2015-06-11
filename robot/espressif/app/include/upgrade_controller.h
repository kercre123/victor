/** @file Upgrade / flash controller
 * This module is responsible for reflashing the Espressif
 * @author Daniel Casner <daniel@anki.com>
 */


/// Initalize the upgrade controller
int8_t upgradeControllerInit(void);

/// Port the espressif will listen on for upgrade commands
#define UPGRADE_CONTROLLER_COMMAND_PORT 8580 ///< = "UP"

/// Header for upgrade command packets
#define UPGRADE_COMMAND_PREFIX "robotsRising"
/// Length of header
#define UPGRADE_COMMAND_PREFIX_LENGTH 12


typedef enum _UpgradeCommandTag
{
  UpgradeCommandNone,        // Dummy, no upgrade
  EspressifFirmwareUpgradeA, // Upgrading the main firmware for the espressif
  EspressifFirmwareUpgradeB, // Upgrading the auxillary espressif firmware
  SysconFirmwareUpgrade,     // Upgrading firmware on the syscon (body) board
  FPGAFirmwareUpgrade,       // Upgrading firmware for the FPGA
} UpgradeCommandTag;

/// Parameters for an firmware upgrade command
typedef struct _UpradeCommandParameters
{
  uint8_t  serverIP[4]; // IP address of the upgrade server
  uint16_t serverPort;  // Port number of ugprade server
  uint16_t command;     // What operation to perform
} UpgradeCommandParameters;
