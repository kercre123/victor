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

#include "wifi-cozmo-img.h"

#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_hash.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "clad/types/birthCertificate.h"
#include "clad/types/imu.h"
#include "../factoryversion.h"

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
static s32 minPositions[4];
static s32 maxPositions[4];
static BirthCertificate birthCert;
static s8  menuIndex;
static void* testModeData;
static int testModePhase;

typedef enum {
  PP_entry = 0,
  PP_pause,
  PP_wipe,
  PP_postWipeDelay,
  PP_setWifi,
  PP_running,
} PlayPenTestState;

extern "C" bool hasBirthCertificate(void) { return birthCert.second != 0xFF; }

#define MENU_TIMEOUT 100000000

static const FTMenuItem factoryMenuItems[] = {
  {"WiFi & Ver info", RobotInterface::FTM_WiFiInfo,      30000000 },
  {"State info",      RobotInterface::FTM_StateMenu,     30000000 },
  {"Motor test",      RobotInterface::FTM_motorLifeTest, 30000000 },
  {"Playpen test",    RobotInterface::FTM_PlayPenTest,   0xFFFFffff },
  {"Cube test",       RobotInterface::FTM_cubeTest,      0xFFFFffff },
  {"IMU calibration", RobotInterface::FTM_IMUCalibration, 120000000 }
#if 0
  {"BLE",             RobotInterface::FTM_BLE_Menu,      15000000 },
#endif
};
#define NUM_FACTORY_MENU_ITEMS (sizeof(factoryMenuItems)/sizeof(FTMenuItem))

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

struct IMUCalibrationState
{
  int32_t  gyroSum[3];
  int32_t  accSum[3];
  int32_t  samples;
  uint32_t flashPos;
  bool     headInPosition;
};

bool Init()
{
  mode = RobotInterface::FTM_None;
  modeTimeout = 0xFFFFffff;
  menuIndex = 0;
  testModeData = NULL;
  testModePhase = 0;
  birthCert.second = 0xFF; // Mark invalid
  return true;
}

static u8 getCurrentMenuItems(const FTMenuItem** items)
{
  switch(mode)
  {
    case RobotInterface::FTM_menus:
    {
      *items = factoryMenuItems;
      return NUM_FACTORY_MENU_ITEMS;
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

static void IMUCalibrationReadCallback(NVStorage::NVStorageBlob* blob, const NVStorage::NVResult result)
{
  if (result != NVStorage::NV_OKAY)
  {
    AnkiDebug( 192, "IMUCalibration.Read.NotFound", 501, "No IMU calibration data available", 0);
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
          AnkiDebug( 193, "IMUCalibrationData.Read.Success", 502, "Got calibration data: gyro={%d, %d, %d}, acc={%d, %d, %d}", 6, calD[run].gyro[0], calD[run].gyro[1], calD[run].gyro[2],
                        calD[run].acc[0], calD[run].acc[1], calD[run].acc[2]);
          RobotInterface::SendMessage(calD[run]);
          return;
        }
      }
    }
    AnkiDebug( 194, "IMUCalibrationData.Read.NotFound", 503, "No IMU calibration data written", 0);
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

static void NVReadDoneCB(NVStorage::NVStorageBlob* entry, const NVStorage::NVResult result)
{
  if (result == NVStorage::NV_NOT_FOUND && FACTORY_FIRMWARE)
  {
    SetMode(RobotInterface::FTM_FAC);
  }
  else
  {
    if (result == NVStorage::NV_OKAY) memcpy(&birthCert, entry->blob, sizeof(BirthCertificate));
    SetMode(RobotInterface::FTM_Sleepy);
  }
  
  foregroundTaskPost(requestIMUCal, 0);
}

static void WipeAllDoneCB(const u32 param, const NVStorage::NVResult rslt)
{
  switch(mode)
  {
    case RobotInterface::FTM_PlayPenTest:
    {
      testModePhase = PP_postWipeDelay;
      lastExecTime = system_get_time();
      i2spiBootloaderCommandDone();
      break;
    }
    default:
    {
      AnkiWarn( 196, "FactoryTests.WipeAllDoneCB", 505, "Unexpected mode", 0);
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
      {
        bool fallThrough = false;
        switch(testModePhase)
        {
          case PP_entry:
          {
            break;
          }
          case PP_pause:
          {
            if (i2spiMessageQueueIsEmpty())
            {
              i2spiSwitchMode(I2SPI_PAUSED);
              NVStorage::WipeAll(0, true, WipeAllDoneCB, true, false);
              testModePhase = PP_wipe;
            }
            break;
          }
          case PP_wipe:
          {
            break;
          }
          case PP_postWipeDelay:
          {
            break;
          }
          case PP_setWifi:
          {
            os_printf("AFIX wifi\r\n");
            struct softap_config ap_config;
            wifi_softap_get_config(&ap_config);
            os_memset(ap_config.ssid, 0, sizeof(ap_config.ssid));
            os_sprintf((char*)ap_config.ssid, "Afix%02d", modeParam & 63);
            ap_config.authmode = AUTH_OPEN;
            ap_config.channel = 11;    // Hardcoded channel - EL (factory) has no traffic here
            ap_config.beacon_interval = 100;
            wifi_softap_set_config_current(&ap_config);
            testModePhase = PP_running;
            os_printf("AFIX running\r\n");
            break;
          }
          case PP_running:
          {
            fallThrough = true;
          }
        }
        if (!fallThrough) break;
        // Else ecplicitly fallthrough to wifi info
      }
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
                           FACTORY_VERSION, BUILD_DATE + 5,
                           FACTORY_VERSION, RTIP::VersionDescription);
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
        if ((now - lastExecTime) >> 19)
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
        const int y = ((now/30000000) % 2) ? 0 : 32;
        u64 frame[COLS];
        Draw::Clear(frame);
        Draw::Copy(frame, PASSWORD_IMG, sizeof(PASSWORD_IMG)/sizeof(PASSWORD_IMG[0]), 0, y);
        Draw::Print(frame, wifiPsk + 0, 4,  2, y+16);
        Draw::Print(frame, wifiPsk + 4, 4, 44, y+16);
        Draw::Print(frame, wifiPsk + 8, 4, 86, y+16);
        Draw::Flip(frame);
        if ((now - lastExecTime) > 300000000)
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
        char ssidNumStr[8];
        os_sprintf(ssidNumStr, "%06X", getSSIDNumber());
        Draw::Copy(frame, SSID_IMG);
        Draw::Print(frame, ssidNumStr, 6, 68, 34);
        //Draw::Mask(frame, columnMask);
        Draw::Flip(frame);
        break;
      }
      case RobotInterface::FTM_FAC:
      {
        {
          using namespace Anki::Cozmo::Face;
          // Display DISPLAY FACTORY WARNING
          const bool odd = (now/1000000) % 2;
          const u64 columnMask = odd ? 0xaaaaaaaaaaaaaaaa : 0x5555555555555555;
          const int xOffset = odd ? 18 : 58;
          u64 frame[COLS];
          Draw::Clear(frame);
          Draw::Number(frame, 3, 0xFAC, xOffset, 8);
          Draw::Invert(frame);
          Draw::Mask(frame, columnMask);
          Draw::Flip(frame);
        }
        if ((now - lastExecTime) > 2000000)
        {
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
        if (now > 300000000)
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
  
  for (int m=0; m<4; ++m)
  {
    if (state.positionsFixed[m] < minPositions[m]) minPositions[m] = state.positionsFixed[m];
    if (state.positionsFixed[m] > maxPositions[m]) maxPositions[m] = state.positionsFixed[m];
  }
  
  switch (mode)
  {
    case RobotInterface::FTM_Sleepy:
    case RobotInterface::FTM_FAC:
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
        if (hasBirthCertificate())
        {
          SetMode(RobotInterface::FTM_WiFiInfo);
          modeTimeout = now + 30000000;
        }
        else
        {
          SetMode(RobotInterface::FTM_menus);
          modeTimeout = now + MENU_TIMEOUT;
        }
        minPositions[3] = state.positionsFixed[3];
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
    #if FACTORY_FIRMWARE
    case RobotInterface::FTM_IMUCalibration:
    {
      if (testModeData == NULL) 
      {
        Face::FacePrintf("IMU Callibration\ntest mode data error");
        SetMode(RobotInterface::FTM_entry);
      }
      else
      {
        static const int32_t GYRO_CAL_THRESHOLD = 100;    // 100=1.5 DPS
        static const int32_t ACC_CAL_THRESHOLD  = 1000;   // 1000=0.06 G
        static const int32_t IMU_CAL_SAMPLES    = 100;
        IMUCalibrationState* test = reinterpret_cast<IMUCalibrationState*>(testModeData);
        if (test->headInPosition == false)
        {
          Face::FacePrintf("Positioning head\r\n");
          if  (((now - lastExecTime) > 2000000) && (ABS(state.speedsFixed[3]) == 0))
          {
            RobotInterface::EngineToRobot msg;
            msg.tag = RobotInterface::EngineToRobot::Tag_moveHead;
            msg.moveHead.speed_rad_per_sec = 1.0f;
            RTIP::SendMessage(msg);
            test->headInPosition = true;
          }
        }
        else if (test->samples < IMU_CAL_SAMPLES)
        {
          int i;
          Face::FacePrintf("Collecting samples\n  %d\n", test->samples);
          if (test->samples <= 0) // Collect 1st sample
          {
            for (i=0; i<3; ++i)
            {
              test->gyroSum[i] = state.gyro[i];
              test->accSum[i]  = state.acc[i];
            }
            test->samples = 1;
          }
          else // Collect more samples and restart if nessisary
          {
            int32_t gyroAvg[3];
            int32_t accAvg[3];
            bool restart = false;
            for (i=0; i<3; ++i)
            {
              gyroAvg[i] = test->gyroSum[i] / test->samples;
              if (ABS(state.gyro[i] - gyroAvg[i]) > GYRO_CAL_THRESHOLD)
              {
                restart = true;
                break;
              }
              accAvg[i]  = test->accSum[i]  / test->samples;
              if (ABS(state.acc[i] - accAvg[i]) > ACC_CAL_THRESHOLD)
              {
                restart = true;
                break;
              }
            }
            if (restart)
            {
              test->samples = 0;
            }
            else
            {
              for (i=0; i<3; ++i)
              {
                test->gyroSum[i] += state.gyro[i];
                test->accSum[i]  += state.acc[i];
              }
              test->samples++;
            }
          }
        }
        else // Store the average
        {
          Face::FacePrintf("Storing result:\n  %d\n", test->flashPos);
          if ((test->flashPos + sizeof(RobotInterface::IMUCalibrationData)) < SECTOR_SIZE) // Haven't run out of space yet
          {
            STORE_ATTR RobotInterface::IMUCalibrationData ce;
            uint32_t* cePtr = reinterpret_cast<uint32_t*>(&ce);
            const uint32_t proposedAddress = ((FIXTURE_STORAGE_SECTOR + 1) * SECTOR_SIZE) + test->flashPos;
            SpiFlashOpResult rslt = spi_flash_read(proposedAddress, cePtr, sizeof(RobotInterface::IMUCalibrationData));
            if (rslt != SPI_FLASH_RESULT_OK)
            {
              AnkiWarn( 189, "FactoryTests.IMUCalibration", 493, "Flash read error %d", 1, rslt);
            }
            for (uint32_t w=0; w<(sizeof(RobotInterface::IMUCalibrationData)/sizeof(uint32_t)); ++w)
            {
              if (cePtr[w] != 0xFFFFffff) // Can't store it here
              {
                test->flashPos += sizeof(RobotInterface::IMUCalibrationData);
                return;
              }
            }
            for (int i=0; i<3; ++i)
            {
              ce.gyro[i] = test->gyroSum[i]/test->samples;
              ce.acc[i]  = test->accSum[i]/test->samples;
            }
            rslt = spi_flash_write(proposedAddress, cePtr, sizeof(RobotInterface::IMUCalibrationData));
            if (rslt != SPI_FLASH_RESULT_OK)
            {
              AnkiWarn( 189, "FactoryTests.IMUCalibration", 494, "Flash write error %d", 1, rslt);
            }
            else
            {
              SetMode(RobotInterface::FTM_entry);
            }
          }
        }
      }
      break;
    }
    #endif
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
    #if FACTORY_FIRMWARE
    case RobotInterface::FTM_cubeTest:
    {
      RTIP::HookWifi(NULL);
      break;
    }
    case RobotInterface::FTM_IMUCalibration:
    {
      Anki::Cozmo::RobotInterface::SetBodyRadioMode bMsg;
      bMsg.radioMode = Anki::Cozmo::RobotInterface::BODY_IDLE_OPERATING_MODE;
      Anki::Cozmo::RobotInterface::SendMessage(bMsg);
      if (testModeData != NULL)
      {
        os_free(testModeData);
        testModeData = NULL;
      }
      break;
    }
    #endif
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
    case RobotInterface::FTM_None:
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableLiftPower;
      msg.enableLiftPower.enable = true;
      const bool success = Anki::Cozmo::RTIP::SendMessage(msg);
      os_printf("Enable lift power %d\r\n", success);
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableTestStateMessage;
      msg.enableTestStateMessage.enable = false;
      Anki::Cozmo::RTIP::SendMessage(msg);
      break;
    }
    case RobotInterface::FTM_entry:
    {
      lastExecTime = system_get_time();
      msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      msg.setBodyRadioMode.radioMode = RobotInterface::BODY_IDLE_OPERATING_MODE;
      RTIP::SendMessage(msg);
      break;
    }
    case RobotInterface::FTM_menus:
    {
      msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      msg.setBodyRadioMode.radioMode = RobotInterface::BODY_ACCESSORY_OPERATING_MODE;
      RTIP::SendMessage(msg);
      // Explicit fall through to next case
    }
    case RobotInterface::FTM_SSID:
    case RobotInterface::FTM_WiFiInfo:
    case RobotInterface::FTM_StateMenu:
    case RobotInterface::FTM_BLE_Menu:
    case RobotInterface::FTM_EncoderInfo:
    case RobotInterface::FTM_Sleepy:
    case RobotInterface::FTM_FAC:
    {
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableLiftPower;
      msg.enableLiftPower.enable = false;
      const bool success = Anki::Cozmo::RTIP::SendMessage(msg);
      os_printf("Disable lift power %d\r\n", success);
      break;
    }
    case RobotInterface::FTM_motorLifeTest:
    {
      msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      msg.setBodyRadioMode.radioMode = RobotInterface::BODY_ACCESSORY_OPERATING_MODE;
      RTIP::SendMessage(msg);
      msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableLiftPower;
      msg.enableLiftPower.enable = true;
      const bool success = Anki::Cozmo::RTIP::SendMessage(msg);
      os_printf("Enable lift power %d\r\n", success);
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
      if (mode != RobotInterface::FTM_PlayPenTest)
      {
        if ((hasBirthCertificate() == false) && (NVStorage::IsFactoryNVClear() == false))
        {
          testModePhase = PP_pause;
          // Enable battery power for wipe all
          msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
          msg.setBodyRadioMode.radioMode = Anki::Cozmo::RobotInterface::BODY_ACCESSORY_OPERATING_MODE;
          RTIP::SendMessage(msg);
          msg.tag = RobotInterface::EngineToRobot::Tag_enterRecoveryMode;
          msg.enterRecoveryMode.mode = RobotInterface::OTA::OTA_Mode;
          RTIP::SendMessage(msg);
        }
        else
        {
          testModePhase = PP_setWifi;
        }
        msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableLiftPower;
        msg.enableLiftPower.enable = true;
        const bool success = Anki::Cozmo::RTIP::SendMessage(msg);
        os_printf("Enable lift power %d\r\n", success);
        msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableTestStateMessage;
        msg.enableTestStateMessage.enable = false;
        Anki::Cozmo::RTIP::SendMessage(msg);
      }
      break;
    }
    case RobotInterface::FTM_IMUCalibration:
    {
      testModeData = os_zalloc(sizeof(IMUCalibrationState));
      if (testModeData != NULL)
      {
        os_memset(testModeData, 0, sizeof(IMUCalibrationState));
        Anki::Cozmo::RobotInterface::SetBodyRadioMode bMsg;
        bMsg.radioMode = Anki::Cozmo::RobotInterface::BODY_ACCESSORY_OPERATING_MODE;
        Anki::Cozmo::RobotInterface::SendMessage(bMsg);
        msg.tag = RobotInterface::EngineToRobot::Tag_moveHead;
        msg.moveHead.speed_rad_per_sec = 3.0f;
        RTIP::SendMessage(msg);
        lastExecTime = system_get_time();
      }
      break;
    }
    #endif
    case RobotInterface::FTM_Off:
    {
      Face::Clear();
      os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
      msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLights;
      RTIP::SendMessage(msg);
      msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
      if (FACTORY_FIRMWARE)
      {
        msg.setBodyRadioMode.radioMode = RobotInterface::BODY_LOW_POWER_OPERATING_MODE;
      }
      else
      {
        msg.setBodyRadioMode.radioMode = RobotInterface::BODY_BLUETOOTH_OPERATING_MODE;
      }
      RTIP::SendMessage(msg);
      WiFiConfiguration::Off(false);
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
