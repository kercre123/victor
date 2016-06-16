/** Impelementation framework for factory test firmware on the Espressif MCU
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "client.h"
#include "driver/i2spi.h"
}
#include "anki/cozmo/robot/logging.h"
#include "anki/common/constantsAndMacros.h"
#include "factoryTests.h"
#include "face.h"
#include "rtip.h"
#include "anki/cozmo/robot/esp.h"
#include "nvStorage.h"
#include "wifi_configuration.h"

extern const unsigned int COZMO_VERSION_COMMIT;
extern const char* DAS_USER;
extern const char* BUILD_DATE;

typedef void (*FTMUpdateFunc)(void);

struct FTMenuItem
{
  const char* name;
  Anki::Cozmo::RobotInterface::FactoryTestMode mode;
  u32 timeout;
};

namespace Anki {
namespace Cozmo {
namespace Factory {

static RobotInterface::FactoryTestMode mode;
static u32 lastExecTime;
static u32 modeTimeout;
static int modeParam;
static s8  menuIndex;

#define MENU_TIMEOUT 100000000

static const FTMenuItem rootMenuItems[] = {
  {"WiFi & Ver info", RobotInterface::FTM_WiFiInfo,      30000000 },
  {"State info",      RobotInterface::FTM_StateMenu,     30000000 },
  {"Motor test",      RobotInterface::FTM_motorLifeTest, 30000000 },
#if FACTORY_FIRMWARE
  {"Playpen test",    RobotInterface::FTM_PlayPenTest,   0xFFFFffff },
  {"Cube test",       RobotInterface::FTM_cubeTest,      0xFFFFffff },
#if 0
  {"BLE",             RobotInterface::FTM_BLE_Menu,      15000000 },
#endif
#endif
};
#define NUM_ROOT_MENU_ITEMS (sizeof(rootMenuItems)/sizeof(FTMenuItem))

static const FTMenuItem stateMenuItems[] = {
  {"ADC info",        RobotInterface::FTM_ADCInfo,       30000000 },
  {"IMU info",        RobotInterface::FTM_ImuInfo,       30000000 },
  {"Encoder info",    RobotInterface::FTM_EncoderInfo,   30000000 },
  {"<--",             RobotInterface::FTM_menus, MENU_TIMEOUT}
};
#define NUM_STATE_MENU_ITEMS (sizeof(stateMenuItems)/sizeof(FTMenuItem))

static const FTMenuItem bleMenuItems[] = {
  {"BLE On",  RobotInterface::FTM_BLE_On,  1000000},
  {"BLE Off", RobotInterface::FTM_BLE_Off, 1000000},
  {"<--",     RobotInterface::FTM_menus,   MENU_TIMEOUT}
};
#define NUM_BLE_MENU_ITEMS (sizeof(bleMenuItems)/sizeof(FTMenuItem))


bool Init()
{
  mode = RobotInterface::FTM_None;
  modeTimeout = 0xFFFFffff;
  menuIndex = 0;
  return true;
}

static u8 getCurrentMenuItems(const FTMenuItem** items)
{
  switch(mode)
  {
    case RobotInterface::FTM_menus:
    {
      *items = rootMenuItems;
      return NUM_ROOT_MENU_ITEMS;
    }
    case RobotInterface::FTM_StateMenu:
    {
      *items = stateMenuItems;
      return NUM_STATE_MENU_ITEMS;
    }
    case RobotInterface::FTM_BLE_Menu:
    {
      *items = bleMenuItems;
      return NUM_BLE_MENU_ITEMS;
    }
    default:
    {
      *items = NULL;
      return 0;
    }
  }
}

static void NVReadDoneCB(NVStorage::NVStorageBlob* entry, const NVStorage::NVResult result)
{
  if (result == NVStorage::NV_NOT_FOUND && FACTORY_FIRMWARE)
  {
    SetMode(RobotInterface::FTM_FAC);
  }
  else
  {
    SetMode(RobotInterface::FTM_Sleepy);
  }
}

void Update()
{
  if (mode != RobotInterface::FTM_None)
  {
    const u32 now = system_get_time();
    if (now > modeTimeout)
    {
      SetMode(RobotInterface::FTM_entry);
      modeTimeout = 0xFFFFffff;
    }
    
    switch (mode)
    {
      case RobotInterface::FTM_entry:
      {
        if (NVStorage::Read(NVStorage::NVEntry_BirthCertificate, NVReadDoneCB) == NVStorage::NV_SCHEDULED)
        {
          SetMode(RobotInterface::FTM_WaitNV);
        }
        break;
      }
      case RobotInterface::FTM_menus:
      case RobotInterface::FTM_StateMenu:
      case RobotInterface::FTM_BLE_Menu:
      {
        char menuBuf[256];
        unsigned int bufIndex = 0;
        const FTMenuItem* items;
        u8 i, numItems;
        numItems = getCurrentMenuItems(&items);
        if (numItems == 0)
        {
          Face::FacePrintf("Invalid menu %d", mode);
        }
        else
        {
          for (i=0; i<numItems; ++i)
          {
            bufIndex += ets_snprintf(menuBuf + bufIndex, sizeof(menuBuf) - bufIndex, "%s %s\n", i==menuIndex ? ">" : " ", items[i].name);
            if (bufIndex >= sizeof(menuBuf))
            {
              bufIndex = sizeof(menuBuf-1);
              break;
            }
          }
          menuBuf[bufIndex] = 0;
          Face::FacePrintf(menuBuf);
        }
        break;
      }
      case RobotInterface::FTM_PlayPenTest:
      case RobotInterface::FTM_WiFiInfo:
      {
        static const char wifiFaceFormat[] ICACHE_RODATA_ATTR STORE_ATTR = "SSID: %s\n"
                                                                          "PSK:  %s\n"
                                                                          "Chan: %d  Stas: %d\n"
                                                                          "WiFi-V: %x\nWiFi-D: %s\n"
                                                                          "RTIP-V: %x\nRTIP-D: %s\n"
                                                                          #if FACTORY_FIRMWARE
                                                                          "FACTORY FIRMWARE"
                                                                          #endif
                                                                          ;
        const uint32 wifiFaceFmtSz = ((sizeof(wifiFaceFormat)+3)/4)*4;
        if (!clientConnected())
        {
          struct softap_config ap_config;
          if (wifi_softap_get_config(&ap_config) == false)
          {
            os_printf("WiFiFace couldn't read back config\r\n");
          }
          char fmtBuf[wifiFaceFmtSz];
          memcpy(fmtBuf, wifiFaceFormat, wifiFaceFmtSz);
          Face::FacePrintf(fmtBuf,
                           ap_config.ssid, ap_config.password, ap_config.channel, wifi_softap_get_station_num(),
                           COZMO_VERSION_COMMIT, BUILD_DATE + 5,
                           RTIP::Version, RTIP::VersionDescription);
        }
        break;
      }
      #if FACTORY_FIRMWARE
      case RobotInterface::FTM_cubeTest:
      {
        Face::FacePrintf("Auto cube");
        break ;
      }
      #endif
      case RobotInterface::FTM_motorLifeTest:
      {
        if ((now - lastExecTime) > 1000000)
        {
          const u32 phase = now >> 20;
          const float wheelSpd = phase & 0x01 ? 200.0f : -200.0f;
          const float liftSpd  = phase & 0x02 ? 1.5f   : -1.5f;
          const float headSpd  = phase & 0x02 ? 2.5f   : -2.5f;
          RobotInterface::EngineToRobot msg;
          msg.tag = RobotInterface::EngineToRobot::Tag_drive;
          msg.drive.lwheel_speed_mmps = wheelSpd;
          msg.drive.rwheel_speed_mmps = wheelSpd;
          msg.drive.lwheel_accel_mmps2 = 500.0f;
          msg.drive.rwheel_accel_mmps2 = 500.0f;
          RTIP::SendMessage(msg);
          msg.tag = RobotInterface::EngineToRobot::Tag_moveLift;
          msg.moveLift.speed_rad_per_sec = liftSpd;
          RTIP::SendMessage(msg);
          msg.tag = RobotInterface::EngineToRobot::Tag_moveHead;
          msg.moveHead.speed_rad_per_sec = headSpd;
          RTIP::SendMessage(msg);
          Face::FacePrintf(phase & 0x01 ? "    --->" : "        <---");
          lastExecTime = now;
        }
        break;
      }
      case RobotInterface::FTM_Sleepy:
      {
        using namespace Anki::Cozmo::Face;
        // Display WiFi password, alternate rows about every 2 minutes
        const u64 columnMask = ((now/30000000) % 2) ? 0xaaaaaaaaaaaaaaaa : 0x5555555555555555;
        u64 frame[COLS];
        // Draw::Copy(frame, Face::SLEEPY_EYES); // XXX: Artwork was not popular with design
        Draw::Number(frame, 8, Face::DecToBCD(wifiPin), 0, 4);
        Draw::Mask(frame, columnMask);
        Draw::Flip(frame);
        if (now > 300000000 && FACTORY_FIRMWARE)
        {
          SetMode(RobotInterface::FTM_Off);
        }
        break;
      }
      case RobotInterface::FTM_FAC:
      {
        {
          using namespace Anki::Cozmo::Face;
          // Display DISPLAY FACTORY WARNING
          const u64 columnMask = ((now/1000000) % 2) ? 0xaaaaaaaaaaaaaaaa : 0x5555555555555555;
          u64 frame[COLS];
          Draw::Clear(frame);
          Draw::Number(frame, 3, 0xFAC, 38, 8);
          Draw::Invert(frame);
          Draw::Mask(frame, columnMask);
          Draw::Flip(frame);
        }
        if ((now - lastExecTime) > 2000000) {
          RobotInterface::EngineToRobot msg;
          os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
          msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLights;
          for (int i=Anki::Cozmo::LED_BACKPACK_FRONT; i<=Anki::Cozmo::LED_BACKPACK_BACK; i++)
          {
            msg.setBackpackLights.lights[i].onColor = Anki::Cozmo::LED_ENC_RED;
            msg.setBackpackLights.lights[i].offColor = Anki::Cozmo::LED_ENC_OFF;
            msg.setBackpackLights.lights[i].onFrames = 30;
            msg.setBackpackLights.lights[i].offFrames = 30;
            msg.setBackpackLights.lights[i].transitionOnFrames = 0;
            msg.setBackpackLights.lights[i].transitionOffFrames = 15;
          }
          RTIP::SendMessage(msg);
          lastExecTime = now;
        }
        if (now > 300000000 && FACTORY_FIRMWARE)
        {
          SetMode(RobotInterface::FTM_Off);
        }
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

void Process_TestState(const RobotInterface::TestState& state)
{
  const u32 now = system_get_time();
  switch (mode)
  {
    case RobotInterface::FTM_entry:
    case RobotInterface::FTM_WaitNV:
    case RobotInterface::FTM_Sleepy:
    case RobotInterface::FTM_FAC:
    case RobotInterface::FTM_Off:
    {
      if ((now > 2000000) && (ABS(state.speedsFixed[0]) > 1000))
      {
        lastExecTime = now;
        SetMode(RobotInterface::FTM_menus);
        modeTimeout = now + MENU_TIMEOUT;
      }
      break;
    }
    case RobotInterface::FTM_menus:
    case RobotInterface::FTM_StateMenu:
    case RobotInterface::FTM_BLE_Menu:
    {
      const FTMenuItem* items;
      const u8 numItems = getCurrentMenuItems(&items);
      if ((now - lastExecTime > 1000000) && numItems)
      {
        if (ABS(state.speedsFixed[0] > 1000))
        {
          lastExecTime = now;
          modeTimeout  = now + MENU_TIMEOUT;
          menuIndex = (menuIndex + 1) % numItems;
        }
        else if (ABS(state.speedsFixed[1] > 1000))
        {
          lastExecTime = now;
          modeTimeout  = now + MENU_TIMEOUT;
          menuIndex = (menuIndex - 1) % numItems;
        }
        else if (state.speedsFixed[2] < -10000)
        {
          lastExecTime = now;
          modeTimeout  = now + MENU_TIMEOUT;
          SetMode(items[menuIndex].mode);
          modeTimeout = system_get_time() + items[menuIndex].timeout;
        }
      }
      break;
    }
    case RobotInterface::FTM_ADCInfo:
    {
      Face::FacePrintf("Cliff: %d\nBatV10x: %d\nExtV10x: %d\nChargeState: 0x%x",
                       state.cliffLevel, state.battVolt10x, state.extVolt10x, state.chargeStat);
      break;
    }
    case RobotInterface::FTM_ImuInfo:
    {
      Face::FacePrintf("Gyro:\n%d\n%d\n%d\nAcc:\n%d\n%d\n%d\n",
                       state.gyro[0], state.gyro[1], state.gyro[2],
                       state.acc[0],  state.acc[1],  state.acc[2]);
      break;
    }
    case RobotInterface::FTM_EncoderInfo:
    {
      Face::FacePrintf("Speeds:\n%d %d\n%d %d\nPositions:\n%d %d\n%d %d\n",
                       (state.speedsFixed[0]),    (state.speedsFixed[1]),
                       (state.speedsFixed[2]),    (state.speedsFixed[3]),
                       (state.positionsFixed[0]), (state.positionsFixed[1]),
                       (state.positionsFixed[2]), (state.positionsFixed[3]));
      break;
    }
    default:
    {
      break;
    }
  }
}

#if FACTORY_FIRMWARE
static void cubeMessageHook(const u8* buffer, int bufferSize) {
  using namespace Anki::Cozmo;

  if (buffer[0] != RobotInterface::RobotToEngine::Tag_activeObjectDiscovered) {
    return ;
  }
  
  RobotInterface::RobotToEngine msg;
  memcpy(msg.GetBuffer(), buffer, bufferSize);

  uint32_t id = msg.activeObjectDiscovered.factory_id;

  static uint32_t slots[7];
  static int slot = 0;

  for (int i = 0; i < slot; i++) {
    if (slots[i] == id) return ;
  }

  RobotInterface::EngineToRobot out;

  out.tag = RobotInterface::EngineToRobot::Tag_setCubeLights; 
  out.setCubeLights.objectID = slot;
  for(int i = 0; i < 4; i++) {
    out.setCubeLights.lights[i].onColor = 
    out.setCubeLights.lights[i].offColor = 0x3DEF;
  }

  if (!RTIP::SendMessage(out)) return ;

  out.tag = RobotInterface::EngineToRobot::Tag_setPropSlot;
  out.setPropSlot.slot = slot;
  out.setPropSlot.factory_id = id;

  if (!RTIP::SendMessage(out)) return ;

  slots[slot] = id;

  if (++slot > 7) SetMode(RobotInterface::FTM_entry);
}
#endif

RobotInterface::FactoryTestMode GetMode()
{
  return mode;
}

int GetParam()
{
  return modeParam;
}

void SetMode(const RobotInterface::FactoryTestMode newMode, const int param)
{
  RobotInterface::EngineToRobot msg;
  Anki::Cozmo::Face::FaceUnPrintf();
  menuIndex = 0;
  
  os_printf("SM %d -> %d\r\n", mode, newMode);
  
  if (mode == RobotInterface::FTM_None && newMode != RobotInterface::FTM_None)
  {
    msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableTestStateMessage;
    msg.enableLiftPower.enable = true;
    Anki::Cozmo::RTIP::SendMessage(msg);
  }
  else if (mode != RobotInterface::FTM_None && newMode == RobotInterface::FTM_None)
  {
    msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableTestStateMessage;
    msg.enableLiftPower.enable = false;
    Anki::Cozmo::RTIP::SendMessage(msg);
  }
  
  // Do cleanup on current mode
  switch (mode)
  {
    case RobotInterface::FTM_entry:
    case RobotInterface::FTM_menus:
    case RobotInterface::FTM_WiFiInfo:
    case RobotInterface::FTM_StateMenu:
    case RobotInterface::FTM_BLE_Menu:
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableLiftPower;
      msg.enableLiftPower.enable = true;
      Anki::Cozmo::RTIP::SendMessage(msg);
      break;
    }
    // #if FACTORY_FIRMWARE
    case RobotInterface::FTM_cubeTest:
    {
      RTIP::HookWifi(NULL);
      break;
    }
    // #endif
    case RobotInterface::FTM_motorLifeTest:
    {
      msg.tag = RobotInterface::EngineToRobot::Tag_stop;
      RTIP::SendMessage(msg);
      break;
    }
    default:
    {
      // Nothing to do
    }
  }
  
  // Do entry to new mode
  switch (newMode)
  {
    case RobotInterface::FTM_entry:
    {
      lastExecTime = system_get_time();
      break;
    }
    case RobotInterface::FTM_menus:
    case RobotInterface::FTM_WiFiInfo:
    case RobotInterface::FTM_StateMenu:
    case RobotInterface::FTM_BLE_Menu:
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableLiftPower;
      msg.enableLiftPower.enable = false;
      Anki::Cozmo::RTIP::SendMessage(msg);
      break;
    }
    case RobotInterface::FTM_BLE_On:
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      msg.setBodyRadioMode.radioMode = Anki::Cozmo::RobotInterface::BODY_BLUETOOTH_OPERATING_MODE;
      Anki::Cozmo::RTIP::SendMessage(msg);
      break;
    }
    case RobotInterface::FTM_BLE_Off:
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      msg.setBodyRadioMode.radioMode = Anki::Cozmo::RobotInterface::BODY_ACCESSORY_OPERATING_MODE;
      Anki::Cozmo::RTIP::SendMessage(msg);
      break;
    }
    #if FACTORY_FIRMWARE
    case RobotInterface::FTM_cubeTest:
    {
      RTIP::HookWifi(cubeMessageHook);
      break;
    }
    case RobotInterface::FTM_PlayPenTest:
    {
      // Create config for test fixture open AP
      struct softap_config ap_config;
      wifi_softap_get_config(&ap_config);
      os_memset(ap_config.ssid, 0, sizeof(ap_config.ssid));
      os_sprintf((char*)ap_config.ssid, "Afix%02d", param);
      ap_config.authmode = AUTH_OPEN;
      ap_config.channel = 11;    // Hardcoded channel - EL (factory) has no traffic here
      ap_config.beacon_interval = 100;
      wifi_softap_set_config_current(&ap_config);
    }
    case RobotInterface::FTM_Off:
    {
      Face::Clear();
      os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
      msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLights;
      RTIP::SendMessage(msg);
      msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      msg.setBodyRadioMode.radioMode = RobotInterface::BODY_IDLE_OPERATING_MODE;
      RTIP::SendMessage(msg);
      WiFiConfiguration::Off(false);
    }
    #endif
    default:
    {
      // Nothing to do
    }
  }
  
  modeParam = param;
  mode = newMode;
}


void Process_EnterFactoryTestMode(const RobotInterface::EnterFactoryTestMode& msg)
{
  modeTimeout = 0xFFFFffff; // Disable timeout
  SetMode(msg.mode);
}

} // Factory
} // Cozmo
} // Anki
