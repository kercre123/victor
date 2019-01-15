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
#include "engine/components/movementComponent.h"

using namespace std;

namespace Anki {
namespace Vector {
  
namespace {
  static constexpr float kTopLeftCornerMagicNumber = 15.0f; 
  static constexpr float kSelectRowStart           = 16.0f; 
  static const     float kTextHorzSpace            = 20.0f; 
  static const     float kTextVertSpace            = 19.0f; 
  static const     float kSelectTextScale          = 0.70f; 
  static const     float kUserTextScaleFullSize    = 0.70f; 
  static const     float kUserTextScaleMidSize     = 0.50f; 
  static const     int   kUserMaxCharsFullSize     = 12;
  static const     int   kUserMaxCharsMidSize      = 16;

  static const     float kAccelThresh              = 0.200f;
  static const     float kAccelOffsetX             = 0.600f;
  static const     int   kDeadZoneTicks            = 7;

  static const     string kWifiSelectPrompt        = "Select Wifi:";
  static const     string kPasswordEntryPrompt     = "<Wifi Password>";
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
   {"A", ACT_APPEND}, {"B", ACT_APPEND}, {"C", ACT_APPEND}, {"D", ACT_APPEND}, {"E", ACT_APPEND}, {"F", ACT_APPEND}, {"G", ACT_APPEND}, {" ..",  ACT_PANEL},
   {"H", ACT_APPEND}, {"I", ACT_APPEND}, {"J", ACT_APPEND}, {"K", ACT_APPEND}, {"L", ACT_APPEND}, {"M", ACT_APPEND}, {"N", ACT_APPEND}, {" Del", ACT_DELETE},
   {"O", ACT_APPEND}, {"P", ACT_APPEND}, {"Q", ACT_APPEND}, {"R", ACT_APPEND}, {"S", ACT_APPEND}, {"T", ACT_APPEND}, {"U", ACT_APPEND}, {" OK",  ACT_NEXT},
   {"V", ACT_APPEND}, {"W", ACT_APPEND}, {"X", ACT_APPEND}, {"Y", ACT_APPEND}, {"Z", ACT_APPEND}, {"-", ACT_APPEND}, {"_", ACT_APPEND}, {" ..",  ACT_PANEL},
  };

PanelCell CellsLcaseLetters[] =
  {
   {"a", ACT_APPEND}, {"b", ACT_APPEND}, {"c", ACT_APPEND}, {"d", ACT_APPEND}, {"e", ACT_APPEND}, {"f", ACT_APPEND}, {"g", ACT_APPEND}, {" ..",  ACT_PANEL},
   {"h", ACT_APPEND}, {"i", ACT_APPEND}, {"j", ACT_APPEND}, {"k", ACT_APPEND}, {"l", ACT_APPEND}, {"m", ACT_APPEND}, {"n", ACT_APPEND}, {" Del", ACT_DELETE},
   {"o", ACT_APPEND}, {"p", ACT_APPEND}, {"q", ACT_APPEND}, {"r", ACT_APPEND}, {"s", ACT_APPEND}, {"t", ACT_APPEND}, {"u", ACT_APPEND}, {" OK",  ACT_NEXT},
   {"v", ACT_APPEND}, {"w", ACT_APPEND}, {"x", ACT_APPEND}, {"y", ACT_APPEND}, {"z", ACT_APPEND}, {"-", ACT_APPEND}, {"_",  ACT_NEXT},  {" ..",  ACT_PANEL},
  };

PanelCell CellsNumbersAndSpecialChars[] =
  {
   {"1", ACT_APPEND}, {"2", ACT_APPEND}, {"3", ACT_APPEND}, {"4", ACT_APPEND}, {"5", ACT_APPEND}, {"6", ACT_APPEND}, {"7", ACT_APPEND}, {" ..",  ACT_PANEL},
   {"8", ACT_APPEND}, {"9", ACT_APPEND}, {"0", ACT_APPEND}, {"-", ACT_APPEND}, {"_", ACT_APPEND}, {",", ACT_APPEND}, {".", ACT_APPEND}, {" Del", ACT_DELETE},
   {"?", ACT_APPEND}, {"/", ACT_APPEND}, {"~", ACT_APPEND}, {"#", ACT_APPEND}, {"@", ACT_APPEND}, {"-", ACT_APPEND}, {"_",  ACT_NEXT},  {" OK",  ACT_NEXT},
  };

PanelCell CellsRemainingSpecialChars[] =
  {
   {"!", ACT_APPEND}, {"@", ACT_APPEND},  {"#", ACT_APPEND}, {"$",  ACT_APPEND}, {"%", ACT_APPEND}, {"^", ACT_APPEND}, {"&", ACT_APPEND}, {" ..",  ACT_PANEL},
   {"*", ACT_APPEND}, {"\\", ACT_APPEND}, {"(", ACT_APPEND}, {")",  ACT_APPEND}, {"{", ACT_APPEND}, {"}", ACT_APPEND}, {"+", ACT_APPEND}, {" Del", ACT_DELETE},
   {"`", ACT_APPEND}, {"'",  ACT_APPEND}, {";", ACT_APPEND}, {"<",  ACT_APPEND}, {">", ACT_APPEND}, {"[", ACT_APPEND}, {"]", ACT_APPEND}, {" OK",  ACT_NEXT},
   {"|", ACT_APPEND}, {"=", ACT_APPEND},  {"~", ACT_APPEND}, {"\"", ACT_APPEND}, {":", ACT_APPEND}, {"-", ACT_APPEND}, {"_", ACT_APPEND}, {" ..",  ACT_PANEL},
  };

PanelCell CellsDone[] =
  {
   {" ..", ACT_PANEL},   
   {"OK",  ACT_NEXT},   
  };

PanelCell CellsWifiSelect[] =
  {
   {"AnkiRobits",      ACT_NEXT}, 
   {"BeagleBone-EDB1", ACT_NEXT}, 
   {"SFWireless",      ACT_PREV}, 
  };

Panel kPanelUcaseLetters           = {4, 8, CellsUcaseLetters};
Panel kPanelLcaseLetters           = {4, 8, CellsLcaseLetters};
Panel kPanelNumbersAndSpecialChars = {3, 8, CellsNumbersAndSpecialChars};
Panel kPanelRemainingSpecialChars  = {4, 8, CellsRemainingSpecialChars};
Panel kPanelDone                   = {2, 1, CellsDone};
Panel kPanelWifiSelect             = {3, 1, CellsWifiSelect};


Panel* PasswordEntryPanels[] =
  {
   &kPanelUcaseLetters,
   &kPanelLcaseLetters,
   &kPanelNumbersAndSpecialChars,
   &kPanelRemainingSpecialChars,
   &kPanelDone,
  };
PanelSet PasswordEntry = {sizeof(PasswordEntryPanels)/sizeof(PasswordEntryPanels[0]), PasswordEntryPanels};


Panel*   WifiSelectPanels[] = { &kPanelWifiSelect };
PanelSet WifiSelect         = {sizeof(WifiSelectPanels)/sizeof(WifiSelectPanels[0]), WifiSelectPanels};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::DynamicVariables::DynamicVariables()
{
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

void BehaviorCubeDrive::RestartAnimation() {
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::OnBehaviorActivated() {
  LOG_WARNING("cube_drive", "OnBehaviorActivated()");

  // reset dynamic variables
  _dVars = DynamicVariables();
  _buttonPressed = false;

  _panelSet   = &WifiSelect;
  _currPanel  = 0;
  _promptText = kWifiSelectPrompt;
  _userText   = "";
  _row        = 0;
  _col        = 0;

  ClearHoldCounts();
  _deadZoneTicksLeft = 0;
  for(int i = 0; i < MAX_DIR_COUNT; i++) {
    _dirCountEventMap[i] = false;
  }
  _dirCountEventMap[4] = true;  // first
  // delay
  for (int i = 0; i < 30; i++) {
    _dirCountEventMap[12 + (2*i)] = true; // fast repeat
  }

  GetBEI().GetRobotInfo().GetMoveComponent().MoveHeadToAngle(0.70 /*rad*/, 1.0 /*speed*/, 1.0 /*accel*/);

  RestartAnimation();
}

void BehaviorCubeDrive::OnBehaviorDeactivated() {
  LOG_WARNING("cube_drive", "OnBehaviorDeactivated()");
  _dVars = DynamicVariables();

  GetBEI().GetRobotInfo().GetMoveComponent().DriveWheels(0.0, 0.0, 1000.0, 1000.0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::BehaviorUpdate() {
  if( IsActivated() ) {
    bool isPressed      = GetBEI().GetRobotInfo().IsPowerButtonPressed();
    bool newButtonEvent = isPressed && !_buttonPressed;
    _buttonPressed      = isPressed;

    float xGs = GetBEI().GetRobotInfo().GetHeadAccelData().x / 9810.0 - kAccelOffsetX;
    float yGs = GetBEI().GetRobotInfo().GetHeadAccelData().y / 9810.0;
    // At most, only one direction can be active on any tick
    int dir = NUM_DIR;  // none
    if (abs(xGs) > abs(yGs)) {
      yGs = 0.0;
    } else {
      xGs = 0.0;
    }
    if ((xGs < (-kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_D; }
    if ((xGs > (+kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_U; }
    if ((yGs < (-kAccelThresh)) && (abs(xGs) < kAccelThresh)) { dir = DIR_R; }
    if ((yGs > (+kAccelThresh)) && (abs(xGs) < kAccelThresh)) { dir = DIR_L; }


    Panel* panel = _panelSet->Panels[_currPanel];

    // Register new scroll event if any direction is held for enough
    // consecutive ticks
    if (_deadZoneTicksLeft > 0) {
      _deadZoneTicksLeft--;
    }

    if ((dir == NUM_DIR) || (_dirHoldCount[dir] == 0)) {
      // add extra "dead zone" after stopping hold count for any direction
      for (int i = 0; i < NUM_DIR; i++) {
        if (_dirHoldCount[i] > 0) {
          _deadZoneTicksLeft = kDeadZoneTicks;
        }
      }
      ClearHoldCounts();
      if (dir != NUM_DIR) {
        _dirHoldCount[dir]++;
      }
    } else {
      _dirHoldCount[dir]++;
      if (_dirCountEventMap[_dirHoldCount[dir]]) {
        NewDirEvent(dir, panel->NumRows-1, panel->NumCols-1);
      }
    }

    //if(GetBEI().GetCubeInteractionTracker().GetTargetStatus().tappedDuringLastTick) {
    if (newButtonEvent) {
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
        _currPanel++;
        if (_currPanel >= _panelSet->NumPanels) {
          _currPanel = 0;
        }
        if (_row >= _panelSet->Panels[_currPanel]->NumRows) {
          _row = _panelSet->Panels[_currPanel]->NumRows-1;
        }
        if (_col >= _panelSet->Panels[_currPanel]->NumCols) {
          _col = _panelSet->Panels[_currPanel]->NumCols-1;
        }
        break;
      case ACT_PREV:
        _userText = "";  // TODO
        break;
      case ACT_NEXT:
        // TEMPORARY lightweight on-boarding flow
        _userText  = "";
        _row       = 0;
        _col       = 0;
        _currPanel = 0;
        if (_promptText != kPasswordEntryPrompt) {
          _panelSet   = &PasswordEntry;
          _promptText = kPasswordEntryPrompt;
        } else {
          _panelSet   = &WifiSelect;
          _promptText = kWifiSelectPrompt;
        }
        break;
      }
    }

    // Update the screen
    _dVars.image = Vision::Image(FACE_DISPLAY_HEIGHT,FACE_DISPLAY_WIDTH, NamedColors::BLACK);
    float headingSize = kUserTextScaleFullSize;
    string heading = (_userText != "") ? _userText : _promptText;
    size_t hlen = heading.length();
    if (hlen > kUserMaxCharsMidSize) {
      heading = "..." + heading.substr(hlen-kUserMaxCharsMidSize+2, string::npos);
      headingSize = kUserTextScaleMidSize;
    } else if (hlen > kUserMaxCharsFullSize) {
      headingSize = kUserTextScaleMidSize;
    }

    _dVars.image.DrawText(Point2f(0, kTopLeftCornerMagicNumber),
                          heading, NamedColors::WHITE, headingSize);
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
