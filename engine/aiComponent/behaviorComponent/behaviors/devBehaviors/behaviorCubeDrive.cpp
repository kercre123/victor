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
  static const     float kTextHorzSpace            = 17.5f; 
  static const     float kTextVertSpace            = 22.0f; 
  static const     float kUserTextScale            = 0.70f; 
  static const     float kSelectTextScale          = 0.70f; 
  static const     float kMinAccel                 = 0.100f;
}

enum {
      ACTION_APPEND = 0,
      ACTION_DELETE = 1,
      ACTION_DONE   = 2
};

struct CursorCell {
  string Text;
  int    Action;
};


// const static int NUM_ROWS = 3;
// const static int NUM_COLS = 4;
// CursorCell CURSOR_MATRIX[NUM_ROWS][NUM_COLS] =
//   {
//    {{"0", ACTION_APPEND},
//     {"1", ACTION_APPEND},
//     {"2", ACTION_APPEND},
//     {"3", ACTION_APPEND}},

//    {{"4", ACTION_APPEND},
//     {"5", ACTION_APPEND},
//     {"6", ACTION_APPEND},
//     {"7", ACTION_APPEND}},

//    {{"8", ACTION_APPEND},
//     {"9", ACTION_APPEND},
//     {"DEL", ACTION_DELETE},
//     {"DONE", ACTION_DONE}},
//   };

// const static int NUM_ROWS = 4;
// const static int NUM_COLS = 10;
// CursorCell CURSOR_MATRIX[NUM_ROWS][NUM_COLS] =
//   {
//    {{"0", ACTION_APPEND},
//     {"1", ACTION_APPEND},
//     {"2", ACTION_APPEND},
//     {"3", ACTION_APPEND},
//     {"4", ACTION_APPEND},
//     {"5", ACTION_APPEND},
//     {"6", ACTION_APPEND},
//     {"7", ACTION_APPEND},
//     {"8", ACTION_APPEND},
//     {"9", ACTION_APPEND}},

//    {{"A", ACTION_APPEND},
//     {"B", ACTION_APPEND},
//     {"C", ACTION_APPEND},
//     {"D", ACTION_APPEND},
//     {"E", ACTION_APPEND},
//     {"F", ACTION_APPEND},
//     {"G", ACTION_APPEND},
//     {"H", ACTION_APPEND},
//     {"I", ACTION_APPEND},
//     {"SH", ACTION_DONE},
//     },

//    {{"J", ACTION_APPEND},
//     {"K", ACTION_APPEND},
//     {"L", ACTION_APPEND},
//     {"M", ACTION_APPEND},
//     {"N", ACTION_APPEND},
//     {"O", ACTION_APPEND},
//     {"P", ACTION_APPEND},
//     {"Q", ACTION_APPEND},
//     {"R", ACTION_APPEND},
//     {"DL", ACTION_DELETE}},

//    {{"S", ACTION_APPEND},
//     {"T", ACTION_APPEND},
//     {"U", ACTION_APPEND},
//     {"V", ACTION_APPEND},
//     {"W", ACTION_APPEND},
//     {"X", ACTION_APPEND},
//     {"Y", ACTION_APPEND},
//     {"Z", ACTION_APPEND},
//     {"-", ACTION_APPEND},
//     {"OK", ACTION_DONE}},
//   };

const static int NUM_ROWS = 3;
const static int NUM_COLS = 10;
CursorCell CURSOR_MATRIX[NUM_ROWS][NUM_COLS] =
  {
   {{"A", ACTION_APPEND},
    {"B", ACTION_APPEND},
    {"C", ACTION_APPEND},
    {"D", ACTION_APPEND},
    {"E", ACTION_APPEND},
    {"F", ACTION_APPEND},
    {"G", ACTION_APPEND},
    {"H", ACTION_APPEND},
    {"I", ACTION_APPEND},
    {"SH", ACTION_DONE},
    },

   {{"J", ACTION_APPEND},
    {"K", ACTION_APPEND},
    {"L", ACTION_APPEND},
    {"M", ACTION_APPEND},
    {"N", ACTION_APPEND},
    {"O", ACTION_APPEND},
    {"P", ACTION_APPEND},
    {"Q", ACTION_APPEND},
    {"R", ACTION_APPEND},
    {"DL", ACTION_DELETE}},

   {{"S", ACTION_APPEND},
    {"T", ACTION_APPEND},
    {"U", ACTION_APPEND},
    {"V", ACTION_APPEND},
    {"W", ACTION_APPEND},
    {"X", ACTION_APPEND},
    {"Y", ACTION_APPEND},
    {"Z", ACTION_APPEND},
    {"-", ACTION_APPEND},
    {"OK", ACTION_DONE}},
  };

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
  _userText = "";
  _row = 0.0;
  _col = 0.0;

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
  Vec3f filter_coeffs(0.2, 0.2, 0.2);
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
    float xGs = _dVars.filtered_cube_accel->x / 9810.0 / 2;
    float yGs = _dVars.filtered_cube_accel->y / 9810.0 / 2;
    if (abs(xGs) < kMinAccel) {
      xGs = 0.0;
    }
    if (abs(yGs) < kMinAccel) {
      yGs = 0.0;
    }

    _col += yGs;
    _row += xGs;
    if (_col < 0.0) {
      _col = 0.0;
    }
    if (_col > float(NUM_COLS-1)) {
      _col = float(NUM_COLS-1);
    }
    if (_row < 0.0) {
      _row = 0.0;
    }
    if (_row > float(NUM_ROWS-1)) {
      _row = float(NUM_ROWS-1);
    }

    // float left_wheel_mmps = -xGs * 250.0;
    // float right_wheel_mmps = -xGs * 250.0;

    // left_wheel_mmps += yGs * 250.0;
    // right_wheel_mmps -= yGs * 250.0;

    // if(abs(left_wheel_mmps)<8.0) {
    //   left_wheel_mmps = 0.0;
    // }
    // if(abs(right_wheel_mmps)<8.0) {
    //   right_wheel_mmps = 0.0;
    // }
     
    // GetBEI().GetRobotInfo().GetMoveComponent().DriveWheels(left_wheel_mmps, right_wheel_mmps, 1000.0, 1000.0);

    // double now = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
    // if(now - 0.25 > _dVars.last_lift_action_time) {
    //   if(_dVars.filtered_cube_accel->z > 9810.0 * 2.0) {
    //     _dVars.last_lift_action_time = now;
    //     SetLiftState(true);
    //   } else if(_dVars.filtered_cube_accel->z < -9810.0) {
    //     _dVars.last_lift_action_time = now;
    //     SetLiftState(false);
    //   }
    // }
    if(GetBEI().GetCubeInteractionTracker().GetTargetStatus().tappedDuringLastTick) {
      // toggle lift up/down
      //_liftIsUp = !_liftIsUp;
      //SetLiftState(_liftIsUp);

      CursorCell cc = CURSOR_MATRIX[int(_row)][int(_col)];
      switch (cc.Action) {
      case ACTION_APPEND:
        _userText += cc.Text;
        break;
      case ACTION_DELETE:
        if (_userText.length() > 0) {
          _userText = _userText.substr(0, _userText.length()-1);
        }
        break;
      case ACTION_DONE:
        _userText = "";  // TODO
        break;
      }
    }

    // Update the screen
    _dVars.image = Vision::Image(FACE_DISPLAY_HEIGHT,FACE_DISPLAY_WIDTH, NamedColors::BLACK);
    _dVars.image.DrawText(Point2f(0, kTopLeftCornerMagicNumber),
                          _userText, NamedColors::WHITE, kUserTextScale);
    // _dVars.image.DrawText(Point2f(0, kTopLeftCornerMagicNumber),
    //                       to_string(int(1000.0 * xGs)), NamedColors::WHITE, kUserTextScale);
    for(int r = 0; r < NUM_ROWS; r++) {
      for (int c = 0; c < NUM_COLS; c++) {
        Point2f p = Point2f((float(c+0) * kTextHorzSpace),
                            (float(r+1) * kTextVertSpace) + kSelectRowStart);
        string t = CURSOR_MATRIX[r][c].Text;
        auto color = NamedColors::RED;
        if ((r == int(_row)) && (c == int(_col))) {
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
