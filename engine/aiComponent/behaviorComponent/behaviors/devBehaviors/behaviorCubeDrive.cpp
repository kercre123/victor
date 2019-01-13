/**
 * File: BehaviorCubeDrive.cpp
 *
 * Author: Ron Barry
 * Created: 2019-01-07
 *
 * Description: A three-dimensional steering wheel - with 8 corners.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/activeObjectAccel.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorCubeDrive.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeAccelComponent.h"
#include "engine/components/cubes/cubeAccelListeners/lowPassFilterListener.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeInteractionTracker.h"
#include "engine/components/movementComponent.h"

using namespace std;

namespace Anki {
namespace Vector {
  
namespace {
  static constexpr float kTopLeftCornerMagicNumber = 15.0f; 
  static constexpr float kSelectRowStart           = 20.0f; 
  static const     float kTextHorzSpace            = 18.5f; 
  static const     float kTextVertSpace            = 22.0f; 
  static const     float kUserTextScale            = 0.70f; 
  static const     float kSelectTextScale          = 0.70f; 
  static const     int   kMaxUserChars             = 13;

  //static const     float kMinAccel                 = 0.100f;
  static const     float kAccelThresh              = 0.150f;  
  // static const     int   kTicksFirstEvent          = 2;
  // static const     int   kTicksPerRepeatSlow       = 7;
  // static const     int   kTicksPerRepeatFast       = 2;
  // static const     int   kMaxSlowEvents            = 2;
};

enum {
      ACT_APPEND = 0,
      ACT_DELETE = 1,
      ACT_PANEL  = 2,  // next panel
      ACT_PREV   = 3,
      ACT_NEXT   = 4
};

PanelCell GetPanelCell(Panel* cmp, int row, int col) {
  int idx = (row * cmp->NumCols) + col;
  return cmp->Cells[idx];
}

PanelCell CellsUcaseLetters[] =
  {
   {"A", ACT_APPEND}, {"B", ACT_APPEND}, {"C", ACT_APPEND}, {"D", ACT_APPEND}, {"E", ACT_APPEND}, {"F", ACT_APPEND}, {"G", ACT_APPEND}, {"H", ACT_APPEND}, {"I", ACT_APPEND}, {"^", ACT_PANEL},   
   {"J", ACT_APPEND}, {"K", ACT_APPEND}, {"L", ACT_APPEND}, {"M", ACT_APPEND}, {"N", ACT_APPEND}, {"O", ACT_APPEND}, {"P", ACT_APPEND}, {"Q", ACT_APPEND}, {"R", ACT_APPEND}, {"<", ACT_DELETE}, 
   {"S", ACT_APPEND}, {"T", ACT_APPEND}, {"U", ACT_APPEND}, {"V", ACT_APPEND}, {"W", ACT_APPEND}, {"X", ACT_APPEND}, {"Y", ACT_APPEND}, {"Z", ACT_APPEND}, {"-", ACT_APPEND}, {">", ACT_NEXT},   
  };

PanelCell CellsLcaseLetters[] =
  {
   {"a", ACT_APPEND}, {"b", ACT_APPEND}, {"c", ACT_APPEND}, {"d", ACT_APPEND}, {"e", ACT_APPEND}, {"f", ACT_APPEND}, {"g", ACT_APPEND}, {"h", ACT_APPEND}, {"i", ACT_APPEND}, {"^", ACT_PANEL},   
   {"j", ACT_APPEND}, {"k", ACT_APPEND}, {"l", ACT_APPEND}, {"m", ACT_APPEND}, {"n", ACT_APPEND}, {"o", ACT_APPEND}, {"p", ACT_APPEND}, {"q", ACT_APPEND}, {"r", ACT_APPEND}, {"<", ACT_DELETE}, 
   {"s", ACT_APPEND}, {"t", ACT_APPEND}, {"u", ACT_APPEND}, {"v", ACT_APPEND}, {"w", ACT_APPEND}, {"x", ACT_APPEND}, {"y", ACT_APPEND}, {"z", ACT_APPEND}, {"-", ACT_APPEND}, {">", ACT_NEXT},   
  };

PanelCell CellsWifiPrompt[] =
  {
   {"* OK",   ACT_NEXT},
   {"* Back", ACT_PREV},
  };

Panel kPanelUcaseLetters = {3, 10, CellsUcaseLetters};
Panel kPanelLcaseLetters = {3, 10, CellsLcaseLetters};
Panel kPanelWifiPrompt   = {2, 1,  CellsWifiPrompt};



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::DynamicVariables::DynamicVariables()
{
  last_lift_action_time = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::BehaviorCubeDrive(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::~BehaviorCubeDrive()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::ClearHoldCounts()
{
  for (int i = 0; i < NUM_DIR; i++) {
    _dirHoldCount[i] = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::NewDirEvent(int dir, int maxRow, int maxCol)
{
  switch (dir) {
  case DIR_U: _row = (_row <= 0)      ? 0      : (_row - 1); break;
  case DIR_D: _row = (_row >= maxRow) ? maxRow : (_row + 1); break;
  case DIR_L: _col = (_col <= 0)      ? 0      : (_col - 1); break;
  case DIR_R: _col = (_col >= maxCol) ? maxCol : (_col + 1); break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCubeDrive::WantsToBeActivatedBehavior() const
{
  // This method will only be called if the "modifiers" configuration caused the parent
  // WantsToBeActivated to return true. IOW, if there is a cube connection, this method
  // will be called. If there is a cube connection, we always want to run. (This won't
  // be true outside of the context of the demo. We'll likely want some toggle to 
  // activate/deactivate.)
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  // This will cause the parent WantsToBeActivated to return false any time there is no cube connection.
  // Here, "Lazy" means that we don't want to manage the connection. That will be handled by the 
  // CubeConnectionCoordinator.
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::RequiredLazy;
  
  // The engine will take control of the cube lights to communicate internal state to the user, unless we
  // indicate that we want ownership. Since we're going to be using the lights to indicate where the "front"
  // of the steering "wheel" is, we don't want that distraction.
  modifiers.connectToCubeInBackground = true;

  // Don't cancel me just because I'm not delegating to someone/thing else.
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  /*
  const char* list[] = {
    // TODO: insert any possible root-level json keys that this class is expecting.
    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
  */
}

void BehaviorCubeDrive::SetLiftState(bool up) {
  _dVars.lift_up = up;
  GetBEI().GetRobotInfo().GetMoveComponent().MoveLiftToHeight(
    up ? LIFT_HEIGHT_CARRY : LIFT_HEIGHT_LOWDOCK, MAX_LIFT_SPEED_RAD_PER_S, MAX_LIFT_ACCEL_RAD_PER_S2, 0.1, nullptr);
}

void BehaviorCubeDrive::RestartAnimation() {
  static std::function<void(void)> restart_animation_callback = std::bind(&BehaviorCubeDrive::RestartAnimation, this);
  bool retval = GetBEI().GetCubeLightComponent().PlayLightAnimByTrigger(
      _dVars.object_id,
      CubeAnimationTrigger::CubeDrive,
      restart_animation_callback
  );  
  LOG_WARNING("cube_drive", "RestartAnimation() PlayLightAnimByTrigerr() returned: %d", retval);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::OnBehaviorActivated() {
  LOG_WARNING("cube_drive", "OnBehaviorActivated()");

  // reset dynamic variables
  _dVars = DynamicVariables();
  _liftIsUp = false;
  SetLiftState(_liftIsUp);
  _promptText = "<Wifi Passwd>";
  _userText   = "";
  _row = 0;
  _col = 0;
  ClearHoldCounts();
  for(int i = 0; i < MAX_DIR_COUNT; i++) {
    _dirCountEventMap[i] = false;
  }
  _dirCountEventMap[2] = true;  // first
  // delay
  _dirCountEventMap[12] = true; // fast repeat
  _dirCountEventMap[14] = true;
  _dirCountEventMap[16] = true;
  _dirCountEventMap[18] = true;
  _dirCountEventMap[20] = true;
  _dirCountEventMap[22] = true;
  _dirCountEventMap[24] = true;
  _dirCountEventMap[26] = true;
  _dirCountEventMap[28] = true;
  _dirCountEventMap[30] = true;
  _dirCountEventMap[32] = true;
  _dirCountEventMap[34] = true;
  _dirCountEventMap[36] = true;
  _dirCountEventMap[38] = true;
  _dirCountEventMap[40] = true;
  _dirCountEventMap[42] = true;
  _dirCountEventMap[44] = true;
  _dirCountEventMap[46] = true;
  _dirCountEventMap[48] = true;

  // Get the ObjectId of the connected cube and hold onto it so we can....
  ActiveID connected_cube_active_id = GetBEI().GetCubeCommsComponent().GetConnectedCubeActiveId();
  ActiveObject* active_object = GetBEI().GetBlockWorld().GetConnectedActiveObjectByActiveID(connected_cube_active_id);
  
  if(active_object == nullptr) {
    LOG_ERROR("cube_drive", "active_object came back NULL: %d", connected_cube_active_id);
    CancelSelf();
    return;
  } 

  _dVars.object_id = active_object->GetID();

  RestartAnimation();

  //Vec3f filter_coeffs(1.0, 1.0, 1.0);
  Vec3f filter_coeffs(0.3, 0.3, 0.3);
  _dVars.filtered_cube_accel = std::make_shared<ActiveAccel>();
  _dVars.low_pass_filter_listener = std::make_shared<CubeAccelListeners::LowPassFilterListener>(
      filter_coeffs,
      _dVars.filtered_cube_accel);
  GetBEI().GetCubeAccelComponent().AddListener(_dVars.object_id, _dVars.low_pass_filter_listener);
}

void BehaviorCubeDrive::OnBehaviorDeactivated() {
  LOG_WARNING("cube_drive", "OnBehaviorDeactivated()");
  _dVars = DynamicVariables();
  SetLiftState(false);

  GetBEI().GetRobotInfo().GetMoveComponent().DriveWheels(0.0, 0.0, 1000.0, 1000.0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::BehaviorUpdate() {
  if( IsActivated() ) {
    if(not GetBEI().GetCubeCommsComponent().IsConnectedToCube()) {
      CancelSelf();
      LOG_ERROR("cube_drive", "We've lost the connection to the cube");
      return;
    }
    float xGs = _dVars.filtered_cube_accel->x / 9810.0;
    float yGs = _dVars.filtered_cube_accel->y / 9810.0;

    // At most, only one direction can be active on any tick
    int dir = NUM_DIR;  // none
    if ((xGs < (-kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_U; }
    if ((xGs > (+kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_D; }
    if ((yGs < (-kAccelThresh)) && (abs(xGs) < kAccelThresh)) { dir = DIR_L; }
    if ((yGs > (+kAccelThresh)) && (abs(xGs) < kAccelThresh)) { dir = DIR_R; }

    Panel* panel = &kPanelUcaseLetters;

    // Register new scroll event if any direction is held for enough
    // consecutive ticks
    if (dir != NUM_DIR) {
      if (_dirHoldCount[dir] == 0) {
        ClearHoldCounts();
      }
      _dirHoldCount[dir]++;

      if (_dirCountEventMap[_dirHoldCount[dir]]) {
        NewDirEvent(dir, panel->NumRows-1, panel->NumCols-1);
      }
    //   int firstHoldCount = _dirHoldCount[dir];
    //   int slowHoldCount  = _dirHoldCount[dir] - kTicksFirstEvent;
    //   int fastHoldCount  = _dirHoldCount[dir] - slowHoldCount - (kTicksPerRepeatSlow * kMaxSlowEvents);
    //   if ((fastHoldCount >= 0) && ((fastHoldCount % kTicksPerRepeatFast) == 0)) {
    //     NewDirEvent(dir, panel->NumRows-1, panel->NumCols-1);
    //   } else if ((slowHoldCount > 0) && (slowHoldCount % kTicksPerRepeatSlow) == 0) {
    //     NewDirEvent(dir, panel->NumRows-1, panel->NumCols-1);
    //   } else if (firstHoldCount == kTicksFirstEvent) {
    //     NewDirEvent(dir, panel->NumRows-1, panel->NumCols-1);
    //   }
    } else {
      ClearHoldCounts();
    }

    if(GetBEI().GetCubeInteractionTracker().GetTargetStatus().tappedDuringLastTick) {
      PanelCell pc = GetPanelCell(panel, _row, _col);
      switch (pc.Action) {
      case ACT_APPEND:
        _userText += pc.Text;
        break;
      case ACT_DELETE:
        if (_userText.length() > 0) {
          _userText = _userText.substr(0, _userText.length()-1);
        }
        break;
      case ACT_PANEL:
        _userText = "";  // TODO
        break;
      case ACT_PREV:
        _userText = "";  // TODO
        break;
      case ACT_NEXT:
        _userText = "";  // TODO
        break;
      }
    }

    // Update the screen
    _dVars.image = Vision::Image(FACE_DISPLAY_HEIGHT,FACE_DISPLAY_WIDTH, NamedColors::BLACK);
    string heading = (_userText != "") ? _userText : _promptText;
    size_t hlen = heading.length();
    if (hlen > kMaxUserChars) {
      heading = "..." + heading.substr(hlen-kMaxUserChars+2, string::npos);
    }
    _dVars.image.DrawText(Point2f(0, kTopLeftCornerMagicNumber),
                          heading, NamedColors::WHITE, kUserTextScale);
    // _dVars.image.DrawText(Point2f(0, kTopLeftCornerMagicNumber),
    //                       to_string(int(1000.0 * xGs)), NamedColors::WHITE, kUserTextScale);
    for(int r = 0; r < panel->NumRows; r++) {
      for (int c = 0; c < panel->NumCols; c++) {
        Point2f p = Point2f((float(c+0) * kTextHorzSpace),
                            (float(r+1) * kTextVertSpace) + kSelectRowStart);
        string t = GetPanelCell(panel, r, c).Text;
        auto color = NamedColors::RED;
        if ((r == _row) && (c == _col)) {
          color = NamedColors::WHITE;
        }
        _dVars.image.DrawText(p, t, color, kSelectTextScale);
      }
    }

    GetBEI().GetAnimationComponent().DisplayFaceImage(_dVars.image, 1.0f, true);
  }
}

} // namespace Vector
} // namespace Anki
