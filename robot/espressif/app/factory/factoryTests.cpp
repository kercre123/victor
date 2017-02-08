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
#include "clad/types/imu.h"

typedef void (*FTMUpdateFunc)(void);

struct FTMenuItem
{
  const char* name;
  Anki::Cozmo::RobotInterface::FactoryTestMode mode;
  u32 timeout;
};

extern "C" { int wifi_setup(const char*, const char*, int); }


namespace Anki {
namespace Cozmo {
namespace Factory {


typedef enum {
  PP_entry = 0,
  PP_pause_DEPRECATED_,
  PP_wipe_DEPRECATED_,
  PP_postWipeDelay_DEPRECATED_,
  PP_setWifi,
  PP_running,
} PlayPenTestState;

typedef enum {
  NO_POS_CHANGE = 0,
  LEFT_TRACK_MOVING = 1<<0,
  RIGHT_TRACK_MOVING = 1<<1,
  LIFT_MOVING = 1<<2,
  HEAD_MOVING = 1<<3,
  LIFT_AT_LIMIT = 1<<4,
  HEAD_AT_LIMIT = 1<<5,
} PositionChangeFlags;


static RobotInterface::FactoryTestMode mode;
static u32 lastExecTime;
static u32 modeTimeout;
static int modeParam;
static s32 minPositions[4];
static s32 maxPositions[4];
static s8  menuIndex;
static u8  extVolt10x;
static PlayPenTestState testModePhase;
static int positionChanges;

#define CHARGER_DETECT_THRESHOLD_DV (40)




  
#define IDLE_SECONDS (1000000)  
#define MENU_TIMEOUT (100*IDLE_SECONDS)
#define TURN_OFF_TIME (30*60*IDLE_SECONDS)
#define NEVER_TIMEOUT (0xFFFFffff)


#if FACTORY_FACE_MENUS
static const FTMenuItem faceMenuItems[] = {
  {"WiFi & Ver info", RobotInterface::FTM_WiFiInfo,      30000000 },
  {"State info",      RobotInterface::FTM_StateMenu,     30000000 },
  {"Motor test",      RobotInterface::FTM_motorLifeTest, 30000000 },
  {"Playpen test",    RobotInterface::FTM_PlayPenTest,   NEVER_TIMEOUT },
#if 0
  {"Cube test",       RobotInterface::FTM_cubeTest,      NEVER_TIMEOUT },
  {"IMU calibration", RobotInterface::FTM_IMUCalibration, 120000000 }
  {"BLE",             RobotInterface::FTM_BLE_Menu,      15000000 },
#endif
};
#else
 
static const FTMenuItem faceMenuItems[] = {
  {"Exit",            RobotInterface::FTM_entry,           NEVER_TIMEOUT },
  {"Power off",       RobotInterface::FTM_batteryCharging, NEVER_TIMEOUT },
  {"Robot info",      RobotInterface::FTM_WiFiInfo,          30000000 },
  {"Sensor info",     RobotInterface::FTM_StateMenu,     MENU_TIMEOUT },
  {"Connections",     RobotInterface::FTM_ConnectionInfo,    30000000 }, 
  {"Factory Reset",   RobotInterface::FTM_FactoryReset,    NEVER_TIMEOUT },
};
#endif
  
#define NUM_ROOT_MENU_ITEMS (sizeof(faceMenuItems)/sizeof(FTMenuItem))

static const FTMenuItem stateMenuItems[] = {
  {"ADC info",        RobotInterface::FTM_ADCInfo,       30000000 },
  {"IMU info",        RobotInterface::FTM_ImuInfo,       30000000 },
#if FACTORY_FACE_MENUS
  {"Encoder info",    RobotInterface::FTM_EncoderInfo,   30000000 },
#endif  
  {"<--",             RobotInterface::FTM_menus, MENU_TIMEOUT}
};
#define NUM_STATE_MENU_ITEMS (sizeof(stateMenuItems)/sizeof(FTMenuItem))

static const FTMenuItem bleMenuItems[] = {
  {"BLE On",  RobotInterface::FTM_BLE_On,  IDLE_SECONDS},
  {"BLE Off", RobotInterface::FTM_BLE_Off, IDLE_SECONDS},
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
  modeTimeout = NEVER_TIMEOUT;
  menuIndex = 0;
  extVolt10x = 50; // We turned on so we must be on the charger
  testModePhase = PP_entry;
  return true;
}

#define IsOnCharger() (extVolt10x > CHARGER_DETECT_THRESHOLD_DV)
#define IdleTimeExceeds(now, usec) (((now)-lastExecTime) > (usec))
#define NotIdle(now)  (lastExecTime = (now))
  
static u8 getCurrentMenuItems(const FTMenuItem** items)
{
  switch(mode)
  {
    case RobotInterface::FTM_menus:
    {
      *items = faceMenuItems;
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


static inline bool NVSRead(NVStorage::NVEntryTag tag,  NVStorage::NVOperationCallback cb)
{
  NVStorage::NVCommand nvc;
  nvc.address = tag;
  nvc.length  = 1;
  nvc.operation = NVStorage::NVOP_READ;
  return (NVStorage::Command(nvc, cb) == NVStorage::NV_SCHEDULED);
}
 


#if 0
static void IMUCalibrationReadCallback(NVStorage::NVOpResult& rslt)
{
  if (rslt.result != NVStorage::NV_OKAY)
  {
    AnkiDebug( 1192, "IMUCalibration.Read.NotFound", 501, "No IMU calibration data available", 0);
  }
  else
  {
    RobotInterface::IMUCalibrationData* calD = reinterpret_cast<RobotInterface::IMUCalibrationData*>(rslt.blob);
    for (int run = (1024/sizeof(RobotInterface::IMUCalibrationData)) - 1; run >= 0; --run)
    {
      uint8_t* bytes = reinterpret_cast<uint8_t*>(&calD[run]);
      for(unsigned int b=0; b<sizeof(RobotInterface::IMUCalibrationData); ++b)
      {
        if (bytes[b] != 0xFF)
        {
          AnkiDebug( 1193, "IMUCalibrationData.Read.Success", 502, "Got calibration data: gyro={%d, %d, %d}, acc={%d, %d, %d}", 6, calD[run].gyro[0], calD[run].gyro[1], calD[run].gyro[2],
                        calD[run].acc[0], calD[run].acc[1], calD[run].acc[2]);
          RobotInterface::SendMessage(calD[run]);
          return;
        }
      }
    }
    AnkiDebug( 1194, "IMUCalibrationData.Read.NotFound", 503, "No IMU calibration data written", 0);
  }
}

static bool requestIMUCal(uint32_t param)
{
  if (!NVCRead(NVStorage::NVEntry_IMUAverages, IMUICalibrationReadCallback))
  {
    os_printf("Failed to request imu calibration data\r\n");
    return true;
  }
  return false;
}
#endif


/* Drawing Helpers */

static void ShowCurrentMenu(void)
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
}

static inline void ShowSSIDAndPwd(const bool highPos)
{
  using namespace Anki::Cozmo::Face;
  u64 frame[COLS];
  const int y = 8 + (1-highPos)*28;

  Draw::Clear(frame);
  Draw::Copy(frame, WIFI_COZMO_IMG, sizeof(WIFI_COZMO_IMG)/sizeof(WIFI_COZMO_IMG[0]), highPos*64, 0);
  Draw::NumberTiny(frame, 6, getSSIDNumber(), 35 + 64*highPos, 0);
  Draw::Copy(frame, WIFI_PASSWORD_IMG, sizeof(WIFI_PASSWORD_IMG)/sizeof(WIFI_PASSWORD_IMG[0]), 0, y);
  Draw::Print(frame, wifiPsk + 0, 4,  2, 16 + y);
  Draw::Print(frame, wifiPsk + 4, 4, 44, 16 + y);
  Draw::Print(frame, wifiPsk + 8, 4, 86, 16 + y);
  Draw::Flip(frame);
}

static inline void ShowFAC(const bool highPos)
{
  using namespace Anki::Cozmo::Face;
  // Display DISPLAY FACTORY WARNING
  const u64 columnMask = highPos ? 0xaaaaaaaaaaaaaaaa : 0x5555555555555555;
  const int xOffset = highPos ? 18 : 58;
  u64 frame[COLS];
  Draw::Clear(frame);
  Draw::Number(frame, 3, 0xFAC, xOffset, 8);
  Draw::Invert(frame);
  Draw::Mask(frame, columnMask);
  Draw::Flip(frame);
}

static void ShowWiFiInfo(uint8_t battVolt10x) {
  static const char wifiFaceFormat[] ICACHE_RODATA_ATTR STORE_ATTR = "SSID: %s\n"
    "PSK:  %s\n"
    "Chan: %d  Stas: %d\n"
    "Ver:  %d%c\n"
    "Time: %d\n"
    "Battery V x10: %d\n"
    "%s";
  
  if (!clientConnected())
  {
    const uint32 wifiFaceFmtSz = ((sizeof(wifiFaceFormat)+3)/4)*4;
    struct softap_config ap_config;
    if (wifi_softap_get_config(&ap_config) == false)
    {
      os_printf("WiFiFace couldn't read back config\r\n");
    }
    char fmtBuf[wifiFaceFmtSz];
    memcpy(fmtBuf, wifiFaceFormat, wifiFaceFmtSz);
    Face::FacePrintf(fmtBuf,
                     ap_config.ssid, ap_config.password, ap_config.channel,
                     wifi_softap_get_station_num(),
                     UpgradeController::GetFirmwareVersion(), UpgradeController::GetBuildType(),
                     UpgradeController::GetBuildTime(), 
                     battVolt10x, (FACTORY_FACE_MENUS?"FACTORY FIRMWARE":""));
  }
}

static inline void ShowConnectionInfo(void)
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
}


/* Messaging Helpers */

static inline void BackPackLightsBlinkRed(void) {
  RobotInterface::EngineToRobot msg;
  os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
  msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLightsMiddle;
  for (int i=Anki::Cozmo::LED_BACKPACK_FRONT; i<=Anki::Cozmo::LED_BACKPACK_BACK; i++)
  {
    msg.setBackpackLightsMiddle.lights[i].onColor = Anki::Cozmo::LED_ENC_RED;
    msg.setBackpackLightsMiddle.lights[i].offColor = Anki::Cozmo::LED_ENC_OFF;
    msg.setBackpackLightsMiddle.lights[i].onFrames = 30;
    msg.setBackpackLightsMiddle.lights[i].offFrames = 30;
    msg.setBackpackLightsMiddle.lights[i].transitionOnFrames = 0;
    msg.setBackpackLightsMiddle.lights[i].transitionOffFrames = 15;
  }
  RTIP::SendMessage(msg);
}

static inline void BackPackLightsYellow(void) {
  RobotInterface::EngineToRobot msg;
  os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
  msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLightsMiddle;
  msg.setBackpackLightsMiddle.lights[1].onColor  = LED_ENC_YELLOW;
  msg.setBackpackLightsMiddle.lights[1].offColor = LED_ENC_YELLOW;
  RTIP::SendMessage(msg);
}
 
  static inline void EnableLiftPower(bool enable) {
    RobotInterface::EngineToRobot msg;
    msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableMotorPower;
    msg.enableMotorPower.motorID = MOTOR_LIFT;
    msg.enableMotorPower.enable = enable;
    Anki::Cozmo::RTIP::SendMessage(msg);
  }

static void RunMotorLifeTest(const u32 phase)
{
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
}


static void inline SetRtipTestState(bool enable) {
  RobotInterface::EngineToRobot msg;
  msg.tag = Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableTestStateMessage;
  msg.enableTestStateMessage.enable = enable;
  Anki::Cozmo::RTIP::SendMessage(msg);
}

static void SetBodyRadioMode(BodyRadioMode mode)
{
  RobotInterface::EngineToRobot msg;
  os_memset(&msg, 0, sizeof(RobotInterface::EngineToRobot));
  msg.tag = RobotInterface::EngineToRobot::Tag_setBodyRadioMode;
  msg.setBodyRadioMode.radioMode = mode;
  RTIP::SendMessage(msg);
}


static void PlayPenStateUpdate(void) {
  switch(testModePhase)
  {
  case PP_entry:
    {
      os_printf("Default WiFi DOWN\r\n");
      //      wifi_set_event_handler_cb(NULL);
      //      wifi_set_opmode_current(NULL_MODE);  //off
      //wifi_softap_dhcps_stop();
      testModePhase = PP_setWifi;
      break;
    }
  case PP_setWifi:
    {
#if 1
      os_printf("AFIX wifi\r\n");
      char ssid[16];
      //      os_memset(ssid, 0, sizeof(ssid));
      os_sprintf(ssid, "Afix%02d", modeParam & 63);
      //Start AIFX wifi with no pwd.
      //- Hardcoded channel 11 - EL (factory) has no traffic here
          wifi_setup(ssid, NULL, 11);
#else          

            os_printf("AFIX wifi\r\n");
            struct softap_config ap_config;
            wifi_softap_get_config(&ap_config);
            os_memset(ap_config.ssid, 0, sizeof(ap_config.ssid));
            os_sprintf((char*)ap_config.ssid, "Afix%02d", modeParam & 63);
            ap_config.authmode = AUTH_OPEN;
            ap_config.channel = 11;    // Hardcoded channel - EL (factory) has no traffic here
            ap_config.beacon_interval = 100;
            wifi_softap_set_config_current(&ap_config);
#endif            
      testModePhase = PP_running;
      os_printf("AFIX running\r\n");
      break;
    }
  case PP_running:
    ShowWiFiInfo(0);

  default:
    {
      
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
      SetMode(RobotInterface::FTM_entry, NEVER_TIMEOUT);
    }
    
    switch (mode)
    {
      case RobotInterface::FTM_entry:
      {
        if (clientConnected() && !FACTORY_FACE_MENUS)  //only Factory FW shows SSID while connected
        {
          SetMode(RobotInterface::FTM_None); 
        }
        else
        {
          if (FACTORY_BIRTH_CERTIFICATE_CHECK_ENABLED && !hasBirthCertificate())
          {
            SetMode(RobotInterface::FTM_FAC); 
          }
          else
          {
            SetMode(FACTORY_FACE_MENUS ? RobotInterface::FTM_SSID : RobotInterface::FTM_Sleepy);
          }
        }
        break;
      }
      case RobotInterface::FTM_SSID:
      {
        //toggle High/low every 30 seconds.
        bool toggle = (now/(30*IDLE_SECONDS)) % 2;
        ShowSSIDAndPwd(toggle);
        // Explicit fall through to next case to allow power off
      }
      case RobotInterface::FTM_Sleepy:
      {
        if ( (!IsOnCharger() && IdleTimeExceeds(now,5*60*IDLE_SECONDS))  // 5 minutes off charger
             || IdleTimeExceeds(now, 30*60*IDLE_SECONDS) )               // 30 minutes on charger
        {
          SetMode(RobotInterface::FTM_Off);
        }
        break;
      }
      case RobotInterface::FTM_Off:
      case RobotInterface::FTM_batteryCharging:
      {
        if (IdleTimeExceeds(now, 5*IDLE_SECONDS) && i2spiMessageQueueIsEmpty())
        {
          system_deep_sleep(0);
        }
        break;
      }
      case RobotInterface::FTM_menus:
      case RobotInterface::FTM_StateMenu:
      case RobotInterface::FTM_BLE_Menu:
      {
        ShowCurrentMenu();
        break;
      }
      case RobotInterface::FTM_FactoryReset:
      {
        // This message will most likely never be seen but it's nice to have in case it takes a minute to reset
        Face::FacePrintf("\n   RESETTING TO FACTORY\n        FIRMWARE\n");
        break;
      }
      case RobotInterface::FTM_PlayPenTest:
      {
        PlayPenStateUpdate();
        
        break;
      }
      case RobotInterface::FTM_cubeTest:
      {
        Face::FacePrintf("Auto cube\n");
        break;
      }
      case RobotInterface::FTM_FAC:
      {
        ShowFAC((now/IDLE_SECONDS) % 2); //toggle pos every second
        if (IdleTimeExceeds(now, 2*IDLE_SECONDS))
        {
          BackPackLightsBlinkRed();
          NotIdle(now);
        }
        if (now > TURN_OFF_TIME)  //This is absolute from when we first powered on.
        {
          SetMode(RobotInterface::FTM_Off);
        }
        break;
      }
      case RobotInterface::FTM_motorLifeTest:
      {
        if ((now - lastExecTime) >> 19)  //TODO: replace with IdleTimeExceeds(now,524287) ?
        {
          RunMotorLifeTest(now>>20);
          NotIdle(now);
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

void resetPositions(const s32* cur)
{
  for (int i=0; i<4; ++i)
  {
    minPositions[i] = cur[i];
    maxPositions[i] = cur[i];
  }
}


static void DetectPositionChange(const RobotInterface::TestState& state)
{
  for (int m=0; m<4; ++m)
  {
    if (state.positionsFixed[m] < minPositions[m]) minPositions[m] = state.positionsFixed[m];
    if (state.positionsFixed[m] > maxPositions[m]) maxPositions[m] = state.positionsFixed[m];
  }
  positionChanges = NO_POS_CHANGE;
  if (ABS(state.speedsFixed[0]) > 1000)  {  positionChanges |= (int)LEFT_TRACK_MOVING; }
  if (ABS(state.speedsFixed[1]) > 1000)  { positionChanges |= RIGHT_TRACK_MOVING; }
  if (ABS(state.speedsFixed[2]) < -10000)  { positionChanges |= LIFT_MOVING; }
  if (ABS(state.speedsFixed[3]) > 0)  { positionChanges |= HEAD_MOVING; }
  if ((state.positionsFixed[2] - maxPositions[2]) < -50000) { positionChanges |= LIFT_AT_LIMIT; }
  if ((state.positionsFixed[3] - minPositions[3]) > 80000) { positionChanges |= HEAD_AT_LIMIT; }
}


static bool chargerStateChanged(uint8_t chargeVolts) {
  bool nowOnCharger = (chargeVolts > CHARGER_DETECT_THRESHOLD_DV);
  return (nowOnCharger!= IsOnCharger());
}


void Process_TestState(const RobotInterface::TestState& state)
{
  const u32 now = system_get_time();
  DetectPositionChange(state);
  
  switch (mode)
  {
    case RobotInterface::FTM_Sleepy:
    case RobotInterface::FTM_Off:
    case RobotInterface::FTM_FAC:
    {
      if (positionChanges & LIFT_AT_LIMIT)
      {
        NotIdle(now);
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_SSID, NEVER_TIMEOUT);
      }
      else if (chargerStateChanged(state.extVolt10x))
        { // If we've gotten on to or off of the charger, reset the timer
          NotIdle(now);
        }
      break;
    }
    case RobotInterface::FTM_SSID:
    {
      if (positionChanges & LIFT_AT_LIMIT) 
      {
        NotIdle(now);
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_Sleepy, NEVER_TIMEOUT);
      }
      else if (positionChanges & HEAD_AT_LIMIT)
      {
        if (FACTORY_BIRTH_CERTIFICATE_CHECK_ENABLED && !hasBirthCertificate() ) {
          NotIdle(now);
          SetMode(RobotInterface::FTM_menus, now + MENU_TIMEOUT);
        }
        else
        {
          NotIdle(now);
          SetMode(RobotInterface::FTM_WiFiInfo, now + (30*IDLE_SECONDS));
        }         
        resetPositions(state.positionsFixed);
      }
      else if ( chargerStateChanged(state.extVolt10x) )
      { // If we've gotten on to or off of the charger, reset the timer
        NotIdle(now);
      }
      break;
    }
    case RobotInterface::FTM_WiFiInfo:
    {
      ShowWiFiInfo(state.battVolt10x);
      if ((positionChanges & HEAD_AT_LIMIT) &&
          !FACTORY_FACE_MENUS) //Factory robots don't have submenus
      {
        NotIdle(now);
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_menus, now+MENU_TIMEOUT);
      }
      break;
    }
    case RobotInterface::FTM_PlayPenTest:
    {
      break;
    }
    case RobotInterface::FTM_ADCInfo:
    {
      Face::FacePrintf("Cliff: %d\nBatV10x: %d\nExtV10x: %d\nChargeState: 0x%x",
                       state.cliffLevel, state.battVolt10x, state.extVolt10x, state.chargeStat);
      if (positionChanges)
      {
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_StateMenu, MENU_TIMEOUT);
      }
      break;
    }
    case RobotInterface::FTM_ImuInfo:
    {
      Face::FacePrintf("Gyro:\n%d\n%d\n%d\nAcc:\n%d\n%d\n%d\n",
                       state.gyro[0], state.gyro[1], state.gyro[2],
                       state.acc[0],  state.acc[1],  state.acc[2]);
      if (positionChanges)
      {
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_StateMenu, MENU_TIMEOUT);
      }
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
    case RobotInterface::FTM_ConnectionInfo:
    {
      ShowConnectionInfo();
      if (positionChanges)
      {
        NotIdle(now);
        resetPositions(state.positionsFixed);
        SetMode(RobotInterface::FTM_menus, MENU_TIMEOUT);
      }
      break;
    }
    
    case RobotInterface::FTM_menus:
    case RobotInterface::FTM_StateMenu:
    case RobotInterface::FTM_BLE_Menu:
    {
      const FTMenuItem* items;
      const u8 numItems = getCurrentMenuItems(&items);
      if (numItems &&  IdleTimeExceeds(now, 1*IDLE_SECONDS) )
      {
        if (positionChanges) {
          NotIdle(now);
          resetPositions(state.positionsFixed);
          modeTimeout  = now + MENU_TIMEOUT;
        }
        if (positionChanges & LEFT_TRACK_MOVING)
        {
          menuIndex += 1;
          if (menuIndex >= numItems) menuIndex = 0;
        }
        else if (positionChanges & RIGHT_TRACK_MOVING)
        {
          menuIndex -= 1;
          if (menuIndex < 0) menuIndex = numItems - 1;;
        }
        else if (positionChanges & LIFT_AT_LIMIT)
        {
          NotIdle(now);
          SetMode(items[menuIndex].mode, now+items[menuIndex].timeout);
          menuIndex = 0; //reset to first item
        }
        break;
      }
    }
    default:
    {
      break;
    }
  }
  
  extVolt10x = state.extVolt10x; // Set at end for comparison in body
}

RobotInterface::FactoryTestMode GetMode()
{
  return mode;
}

int GetParam()
{
  return modeParam;
}


void SetMode(const RobotInterface::FactoryTestMode newMode, uint32_t timeout, const int param)
{
  Anki::Cozmo::Face::FaceUnPrintf();
  
  
  // Do cleanup on current mode
  switch (mode)
  {
    case RobotInterface::FTM_None:
    {
      SetRtipTestState(true);
      break;
    }
  case RobotInterface::FTM_motorLifeTest:
    {
      EnableLiftPower(false);
      RobotInterface::EngineToRobot msg;
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
      os_memset(minPositions, 0, sizeof(minPositions));
      os_memset(maxPositions, 0, sizeof(maxPositions));
      NotIdle(system_get_time());
      break;
    }
    case RobotInterface::FTM_None:
    {
      SetRtipTestState(false);
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
      //ClearBackPack Lights?
      SetBodyRadioMode(BODY_IDLE_OPERATING_MODE);
      break;
    }
    case RobotInterface::FTM_batteryCharging:
    {
      Face::Clear();
      BackPackLightsYellow();
      
      Face::Clear();
      SetBodyRadioMode(BODY_BATTERY_CHARGE_TEST_MODE);
      break;
    }
    case RobotInterface::FTM_FactoryReset:
    {
      RobotInterface::EngineToRobot msg;
      msg.tag = RobotInterface::EngineToRobot::Tag_killBodyCode;
      RTIP::SendMessage(msg);
      break;
    }
    case RobotInterface::FTM_PlayPenTest:
    {
      testModePhase = PP_entry;
      //      SetRtipTestState(false);
      break;
    }
  case RobotInterface::FTM_motorLifeTest:
    {
      SetBodyRadioMode(BODY_ACCESSORY_OPERATING_MODE);
      EnableLiftPower(true);
      break;
    }
    
    default:
    {
      // Nothing to do
    }
  }
  os_printf("SM %d -> %d (%d)\r\n", mode, newMode, testModePhase);
  
  
  modeParam = param;
  mode = newMode;
  if (timeout) modeTimeout = timeout;
}


void Process_EnterFactoryTestMode(const RobotInterface::EnterFactoryTestMode& msg)
{
  os_printf("Enter factory Test mode with param %d\r\n", msg.param);
  SetMode(msg.mode, NEVER_TIMEOUT, msg.param);
}

} // Factory
} // Cozmo
} // Anki
