/** Impelementation framework for factory test firmware on the Espressif MCU
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "client.h"
}
#include "anki/cozmo/robot/logging.h"
#include "anki/common/constantsAndMacros.h"
#include "factoryTests.h"
#include "face.h"
#include "rtip.h"
#include "anki/cozmo/robot/esp.h"

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

static const u32 faceSpinner[] ICACHE_RODATA_ATTR STORE_ATTR = {'|', '/', '-', '\\', '|', '/', '-', '\\'};
static RobotInterface::FactoryTestMode mode;
static u32 lastExecTime;
static u32 modeTimeout;
static s8  menuIndex;

#define MENU_TIMEOUT 100000000

static const FTMenuItem rootMenuItems[] = {
  {"WiFi & Ver info", RobotInterface::FTM_WiFiInfo,      10000000 },
  {"State info",      RobotInterface::FTM_StateMenu,     30000000 },
  {"Motor test",      RobotInterface::FTM_motorLifeTest, 30000000 },
};
#define NUM_ROOT_MENU_ITEMS (sizeof(rootMenuItems)/sizeof(FTMenuItem))

static const FTMenuItem stateMenuItems[] = {
  {"ADC info",        RobotInterface::FTM_ADCInfo,       30000000 },
  {"IMU info",        RobotInterface::FTM_ImuInfo,       30000000 },
  {"Encoder info",    RobotInterface::FTM_EncoderInfo,   30000000 },
  {"<--",             RobotInterface::FTM_menus, MENU_TIMEOUT}
};
#define NUM_STATE_MENU_ITEMS (sizeof(stateMenuItems)/sizeof(FTMenuItem))

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
    default:
    {
      *items = NULL;
      return 0;
    }
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
        const u32 phase = (now >> 16) & 0x7;
        Face::FacePrintf("\n\n\n\n     %c         %c\n", faceSpinner[phase], faceSpinner[7-((phase-1) & 0x7)]);
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
      case RobotInterface::FTM_WiFiInfo:
      {
        static const char wifiFaceFormat[] ICACHE_RODATA_ATTR STORE_ATTR = "SSID: %s\nPSK:  %s\nChan: %d  Stas: %d\nWiFi-V: %x\nWiFi-D: %s\nRTIP-V: %x\nRTIP-D: %s\n          %c";
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
                           RTIP::Version, RTIP::VersionDescription, faceSpinner[system_get_time() >> 16 & 0x7]);
        }
        break;
      }
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
    {
      if (((now - lastExecTime) > 2000000) && (ABS(state.speedsFixed[0]) > 1000))
      {
        lastExecTime = now;
        SetMode(RobotInterface::FTM_menus);
        modeTimeout = now + MENU_TIMEOUT;
      }
      break;
    }
    case RobotInterface::FTM_menus:
    case RobotInterface::FTM_StateMenu:
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

RobotInterface::FactoryTestMode GetMode()
{
  return mode;
}

void SetMode(const RobotInterface::FactoryTestMode newMode)
{
  RobotInterface::EngineToRobot msg;
  Anki::Cozmo::Face::FaceUnPrintf();
  menuIndex = 0;
  
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
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableLiftPower;
      msg.enableLiftPower.enable = true;
      Anki::Cozmo::RTIP::SendMessage(msg);
      break;
    }
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
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableLiftPower;
      msg.enableLiftPower.enable = false;
      Anki::Cozmo::RTIP::SendMessage(msg);
      break;
    }
    default:
    {
      // Nothing to do
    }
  }
  
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
