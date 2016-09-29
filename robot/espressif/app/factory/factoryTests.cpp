/** Impelementation framework for factory test firmware on the Espressif MCU
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "client.h"
#include "driver/factoryData.h"
#include "driver/i2spi.h"
#include "foregroundTask.h"
#include "user_config.h"  
}
#include "anki/cozmo/robot/logging.h"
#include "anki/common/constantsAndMacros.h"
#include "factoryTests.h"
#include "face.h"
#include "rtip.h"
#include "anki/cozmo/robot/esp.h"
#include "nvStorage.h"
#include "wifi_configuration.h"
#include "upgradeController.h"

#include "wifi-cozmo-img.h"

#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_hash.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "clad/types/birthCertificate.h"
#include "clad/types/imu.h"

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
static s32 minPositions[4];
static s32 maxPositions[4];
static BirthCertificate birthCert;
static s8  menuIndex;

typedef enum {
  PP_entry = 0,
  PP_pause,
  PP_wipe,
  PP_postWipeDelay,
  PP_setWifi,
  PP_running,
} PlayPenTestState;


#define INVALID_BIRTH_SECOND (0x81)

extern "C" bool hasBirthCertificate(void) { return birthCert.second != INVALID_BIRTH_SECOND; }

#define MENU_TIMEOUT 100000000

static const FTMenuItem rootMenuItems[] = {
  {"Exit",            RobotInterface::FTM_entry,           0xFFFFffff },
  {"Power off",       RobotInterface::FTM_batteryCharging, 0xFFFFffff },
  {"Robot info",      RobotInterface::FTM_WiFiInfo,          30000000 },
  {"Sensor info",     RobotInterface::FTM_StateMenu,     MENU_TIMEOUT },
  {"Connections",     RobotInterface::FTM_ConnectionInfo,    30000000 },
  {"Factory Reset",   RobotInterface::FTM_FactoryReset,    0xFFFFffff },
};
#define NUM_ROOT_MENU_ITEMS (sizeof(rootMenuItems)/sizeof(FTMenuItem))

static const FTMenuItem stateMenuItems[] = {
  {"ADC info",        RobotInterface::FTM_ADCInfo,           30000000 },
  {"IMU info",        RobotInterface::FTM_ImuInfo,           30000000 },
  {"<--",             RobotInterface::FTM_menus,         MENU_TIMEOUT }
};
#define NUM_STATE_MENU_ITEMS (sizeof(stateMenuItems)/sizeof(FTMenuItem))

bool Init()
{
  mode = RobotInterface::FTM_None;
  modeTimeout = 0xFFFFffff;
  birthCert.second = INVALID_BIRTH_SECOND; // Mark invalid
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
    default:
    {
      *items = NULL;
      return 0;
    }
  }
}



static void BirthCertificateReadCallback(NVStorage::NVOpResult& rslt)
{
  if (rslt.result == NVStorage::NV_OKAY) memcpy(&birthCert, rslt.blob, sizeof(BirthCertificate));
  SetMode(RobotInterface::FTM_Sleepy);
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
        if (clientConnected())
        {
          SetMode(RobotInterface::FTM_None);
        }
        else
        {
          NVStorage::NVCommand nvc;
          nvc.address = NVStorage::NVEntry_BirthCertificate;
          nvc.length  = 1;
          nvc.operation = NVStorage::NVOP_READ;
          if (NVStorage::Command(nvc, BirthCertificateReadCallback) == NVStorage::NV_SCHEDULED)
          {
            SetMode(RobotInterface::FTM_WaitNV);
          }
        }
        break;
      }
      case RobotInterface::FTM_SSID:
      {
        using namespace Anki::Cozmo::Face;
        u64 frame[COLS];
        const bool odd = ((now/30000000) % 2);
        const int y = 8 + (1-odd)*28;
                
        Draw::Clear(frame);
        Draw::Copy(frame, WIFI_COZMO_IMG, sizeof(WIFI_COZMO_IMG)/sizeof(WIFI_COZMO_IMG[0]), odd*64, 0);
        Draw::NumberTiny(frame, 6, getSSIDNumber(), 35 + 64*odd, 0);
        Draw::Copy(frame, WIFI_PASSWORD_IMG, sizeof(WIFI_PASSWORD_IMG)/sizeof(WIFI_PASSWORD_IMG[0]), 0, y);
        Draw::Print(frame, wifiPsk + 0, 4,  2, 16 + y);
        Draw::Print(frame, wifiPsk + 4, 4, 44, 16 + y);
        Draw::Print(frame, wifiPsk + 8, 4, 86, 16 + y);
        Draw::Flip(frame);
        // Explicit fall through to next case to keep power off
      }
      case RobotInterface::FTM_Sleepy:
      {
        if ((now - lastExecTime) > (30*60*1000000)) // 30 minutes
        {
          SetMode(RobotInterface::FTM_Off);
        }
        break;
      }
      case RobotInterface::FTM_Off:
      case RobotInterface::FTM_batteryCharging:
      {
        if (((now - lastExecTime) > 5000000) && i2spiMessageQueueIsEmpty()) system_deep_sleep(0);
        break;
      }
      case RobotInterface::FTM_menus:
      case RobotInterface::FTM_StateMenu:
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
          if (menuIndex >= numItems) menuIndex = 0;
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
      case RobotInterface::FTM_FactoryReset:
      {
        // This message will most likely never be seen but it's nice to have in case it takes a minute to reset
        Face::FacePrintf("\n   RESETTING TO FACTORY\n        FIRMWARE\n");
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

void resetPositions(const s32* cur)
{
  for (int i=0; i<4; ++i)
  {
    minPositions[i] = cur[i];
    maxPositions[i] = cur[i];
  }
}

void Process_TestState(const RobotInterface::TestState& state)
{
  const u32 now = system_get_time();
  
  for (int m=0; m<4; ++m)
  {
    if (state.positionsFixed[m] < minPositions[m]) minPositions[m] = state.positionsFixed[m];
    if (state.positionsFixed[m] > maxPositions[m]) maxPositions[m] = state.positionsFixed[m];
  }
  
  switch (mode)
  {
    case RobotInterface::FTM_Sleepy:
    case RobotInterface::FTM_Off:
    {
      if ((state.positionsFixed[2] - maxPositions[2]) < -50000)
      {
        lastExecTime = now;
        modeTimeout = 0xFFFFffff;
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_SSID);
      }
      break;
    }
    case RobotInterface::FTM_SSID:
    {
      if ((state.positionsFixed[2] - maxPositions[2]) < -50000)
      {
        lastExecTime = now;
        modeTimeout = 0xFFFFffff;
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_Sleepy);
      }
      else if ((state.positionsFixed[3] - minPositions[3]) > 80000)
      {
        lastExecTime = now;
        modeTimeout = now + 30000000;
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_WiFiInfo);
      }
      break;
    }
    case RobotInterface::FTM_WiFiInfo:
    {
      static const char wifiFaceFormat[] ICACHE_RODATA_ATTR STORE_ATTR = "SSID: %s\n"
                                                                        "PSK:  %s\n"
                                                                        "Chan: %d  Stas: %d\n"
                                                                        "FW Build: %d\n"
                                                                        "FW Date:  %d\n"
                                                                        "Battery V x10: %d";
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
                         UpgradeController::GetFirmwareVersion(), UpgradeController::GetBuildTime(),
                         state.battVolt10x);
      }
      if ((state.positionsFixed[3] - minPositions[3]) > 80000)
      {
        lastExecTime = now;
        modeTimeout = now + MENU_TIMEOUT;
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_menus);
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
    case RobotInterface::FTM_ConnectionInfo:
    {
      char menuBuf[256]="";
      unsigned int bufIndex = 0;
      u8 nc = wifi_softap_get_station_num();
      bufIndex += ets_snprintf(menuBuf + bufIndex, sizeof(menuBuf) - bufIndex, "%d connections\n", nc);
      for (nc = 0;nc < AP_MAX_CONNECTIONS;nc++)
      {
        bufIndex += ets_snprintf(menuBuf + bufIndex, sizeof(menuBuf) - bufIndex,
                                 MACSTR "\n", MAC2STR(connections[nc].mac));
      }
      Face::FacePrintf(menuBuf);
      break;
    }
    
    case RobotInterface::FTM_menus:
    case RobotInterface::FTM_StateMenu:
    {
      const FTMenuItem* items;
      const u8 numItems = getCurrentMenuItems(&items);
      if ((now - lastExecTime > 1000000) && numItems)
      {
        if (ABS(state.speedsFixed[0]) > 1000)
        {
          lastExecTime = now;
          modeTimeout  = now + MENU_TIMEOUT;
          menuIndex += 1;
          if (menuIndex >= numItems) menuIndex = numItems - 1;
        }
        else if (ABS(state.speedsFixed[1]) > 1000)
        {
          lastExecTime = now;
          modeTimeout  = now + MENU_TIMEOUT;
          menuIndex -= 1;
          if (menuIndex < 0) menuIndex = 0;
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
    default:
    {
      break;
    }
  }
}

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
  
  os_printf("SM %d -> %d\r\n", mode, newMode);
  
  // Do cleanup on current mode
  switch (mode)
  {
    case RobotInterface::FTM_None:
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableTestStateMessage;
      msg.enableTestStateMessage.enable = true;
      Anki::Cozmo::RTIP::SendMessage(msg);
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
      os_memset(minPositions, 0, sizeof(minPositions));
      os_memset(maxPositions, 0, sizeof(maxPositions));
      lastExecTime = system_get_time();
      break;
    }
    case RobotInterface::FTM_None:
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableTestStateMessage;
      msg.enableTestStateMessage.enable = false;
      Anki::Cozmo::RTIP::SendMessage(msg);
      break;
    }
    case RobotInterface::FTM_Sleepy:
    {
      Face::Clear();
      break;
    }
    case RobotInterface::FTM_Off:
    {
      Face::Clear();
      os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
      msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLightsMiddle;
      RTIP::SendMessage(msg);
      
      os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
      msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLightsTurnSignals;
      RTIP::SendMessage(msg);

      os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
      msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      msg.setBodyRadioMode.radioMode = BODY_IDLE_OPERATING_MODE;
      RTIP::SendMessage(msg);
      break;
    }
    case RobotInterface::FTM_batteryCharging:
    {
      Face::Clear();
      os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
      msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLightsMiddle;
      msg.setBackpackLightsMiddle.lights[1].onColor  = LED_ENC_YELLOW;
      msg.setBackpackLightsMiddle.lights[1].offColor = LED_ENC_YELLOW;
      RTIP::SendMessage(msg);
      
      Face::Clear();
      os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
      msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      msg.setBodyRadioMode.radioMode = BODY_BATTERY_CHARGE_TEST_MODE;
      RTIP::SendMessage(msg);
      break;
    }
    case RobotInterface::FTM_FactoryReset:
    {
      msg.tag = RobotInterface::EngineToRobot::Tag_killBodyCode;
      RTIP::SendMessage(msg);
      break;
    }
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
  SetMode(msg.mode, msg.param);
}

} // Factory
} // Cozmo
} // Anki
