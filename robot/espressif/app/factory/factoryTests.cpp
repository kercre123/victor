/** Impelementation framework for factory test firmware on the Espressif MCU
 * @author Daniel Casner <daniel@anki.com>
 */

extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "client.h"
#include "driver/crash.h"
#include "driver/i2spi.h"
#include "foregroundTask.h"
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

typedef enum {
  PP_entry = 0,
  PP_pause,
  PP_wipe,
  PP_postWipeDelay,
  PP_setWifi,
  PP_running,
} PlayPenTestState;

extern "C" bool hasBirthCertificate(void) { return birthCert.second != 0xFF; }

bool Init()
{
  mode = RobotInterface::FTM_None;
  modeTimeout = 0xFFFFffff;
  birthCert.second = 0xFF; // Mark invalid
  return true;
}

static void IMUCalibrationReadCallback(NVStorage::NVStorageBlob* blob, const NVStorage::NVResult result)
{
  if (result != NVStorage::NV_OKAY)
  {
    AnkiDebug( 200, "IMUCalibration.Read.NotFound", 501, "No IMU calibration data available", 0);
  }
  else
  {
    RobotInterface::IMUCalibrationData* calD = reinterpret_cast<RobotInterface::IMUCalibrationData*>(blob->blob);
    for (int run = (1024/sizeof(RobotInterface::IMUCalibrationData)) - 1; run >= 0; --run)
    {
      uint8_t* bytes = reinterpret_cast<uint8_t*>(&calD[run]);
      for(unsigned int b=0; b<sizeof(RobotInterface::IMUCalibrationData); ++b)
      {
        if (bytes[b] != 0xFF)
        {
          AnkiDebug( 201, "IMUCalibrationData.Read.Success", 502, "Got calibration data: gyro={%d, %d, %d}, acc={%d, %d, %d}", 6, calD[run].gyro[0], calD[run].gyro[1], calD[run].gyro[2],
                        calD[run].acc[0], calD[run].acc[1], calD[run].acc[2]);
          RobotInterface::SendMessage(calD[run]);
          return;
        }
      }
    }
    AnkiDebug( 202, "IMUCalibrationData.Read.NotFound", 503, "No IMU calibration data written", 0);
  }
}

static bool requestIMUCal(uint32_t param)
{
  if (NVStorage::Read(NVStorage::NVEntry_IMUAverages, IMUCalibrationReadCallback) != NVStorage::NV_SCHEDULED)
  {
    os_printf("Failed to request imu calibration data\r\n");
    return true;
  }
  return false;
}

static void BirthCertificateReadCallback(NVStorage::NVStorageBlob* entry, const NVStorage::NVResult result)
{
  if (result == NVStorage::NV_OKAY) memcpy(&birthCert, entry->blob, sizeof(BirthCertificate));
  SetMode(RobotInterface::FTM_Sleepy);
  clientAccept(true);
  foregroundTaskPost(requestIMUCal, 0);
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
        if (NVStorage::Read(NVStorage::NVEntry_BirthCertificate, BirthCertificateReadCallback) == NVStorage::NV_SCHEDULED)
        {
          SetMode(RobotInterface::FTM_WaitNV);
        }
        break;
      }
      case RobotInterface::FTM_Sleepy:
      {
        if ((now - lastExecTime) < (30*60*1000000)) // 30 minutes
        {
          using namespace Anki::Cozmo::Face;
          // Display WiFi password, alternate rows about every 20 seconds
          const u64 columnMask = ((now/20 * 1000000) % 2) ? 0xaaaaaaaaaaaaaaaa : 0x5555555555555555;
          u64 frame[COLS];
          Draw::Clear(frame);
          Draw::Number(frame, 8, Face::DecToBCD(wifiPin), 0, 4);
          Draw::Mask(frame, columnMask);
          Draw::Flip(frame);
        }
        else
        {
          SetMode(RobotInterface::FTM_Off);
        }
        break;
      }
      case RobotInterface::FTM_SSID:
      {
        using namespace Anki::Cozmo::Face;
        // Display WiFi password, alternate rows about every 2 minutes
        u64 frame[COLS];
        Draw::Copy(frame, SSID_IMG);
        Draw::Number(frame, 6, getSSIDNumber(), 68, 34, false);
        //Draw::Mask(frame, columnMask);
        Draw::Flip(frame);
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
      if ((state.positionsFixed[2] - maxPositions[2]) < -60000)
      {
        lastExecTime = now;
        SetMode(RobotInterface::FTM_SSID);
        modeTimeout = now + 10000000;
        for (int i=0; i<4; ++i)
        {
          minPositions[i] = state.positionsFixed[i];
          maxPositions[i] = state.positionsFixed[i];
        }
      }
      break;
    }
    case RobotInterface::FTM_SSID:
    {
      if ((state.positionsFixed[2] - maxPositions[2]) < -60000)
      {
        lastExecTime = now;
        SetMode(RobotInterface::FTM_Sleepy);
        modeTimeout = 0xFFFFffff;
        for (int i=0; i<4; ++i)
        {
          minPositions[i] = state.positionsFixed[i];
          maxPositions[i] = state.positionsFixed[i];
        }
      }
      else if ((state.positionsFixed[3] - minPositions[3]) > 70000)
      {
        lastExecTime = now;
        SetMode(RobotInterface::FTM_WiFiInfo);
        modeTimeout = now + 30000000;
        minPositions[3] = state.positionsFixed[3];
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
      break;
    }
    case RobotInterface::FTM_None:
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableTestStateMessage;
      msg.enableTestStateMessage.enable = false;
      Anki::Cozmo::RTIP::SendMessage(msg);
      break;
    }
    case RobotInterface::FTM_Off:
    {
      Face::Clear();
      os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
      msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLights;
      RTIP::SendMessage(msg);

      os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
      msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      msg.setBodyRadioMode.radioMode = BODY_IDLE_OPERATING_MODE;
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
