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
  static constexpr float kSelectRowStart           = 15.0f; 
  static const     float kTextHorzSpace            = 20.0f; 
  static const     float kTextVertSpace            = 19.0f; 
  static const     float kSelectTextScale          = 0.70f; 
  static const     float kUserTextScaleFullSize    = 0.70f; 
  static const     float kUserTextScaleMidSize     = 0.50f; 
  static const     int   kNumSelectRows            = 4; // number of rows that can be displayed at once

  static const     float kAccelThresh              = 0.200f;
  static const     float kAccelOffsetX             = 0.650f;
  static const     int   kDeadZoneTicks            = 7;

  static const     string kWifiSelectPrompt        = "Select Wifi:";
  static const     string kPasswordEntryPrompt     = "<Wifi Password>";
};

enum {
      ACT_NOP    = 0, 
      ACT_APPEND = 1, 
      ACT_DELETE = 2, 
      ACT_PANEL  = 3, // next panel 
      ACT_DONE   = 4  
};

enum {
      CTRL_45_REG = 0, 
      CTRL_45_INV = 1, 
      CTRL_HORZ   = 2, 
      NUM_CTRL    = 3,
};

PanelCell GetPanelCell(Panel* cmp, int row, int col) {
  int idx = (row * cmp->NumCols) + col;
  return cmp->Cells[idx];
}

PanelCell CellsAllChars[] =
  {
   {"A",  ACT_APPEND}, {"B", ACT_APPEND}, {"C", ACT_APPEND}, {"D", ACT_APPEND}, {"E", ACT_APPEND}, {"F",  ACT_APPEND}, {"G", ACT_APPEND}, 
   {"H",  ACT_APPEND}, {"I", ACT_APPEND}, {"J", ACT_APPEND}, {"K", ACT_APPEND}, {"L", ACT_APPEND}, {"M",  ACT_APPEND}, {"N", ACT_APPEND}, 
   {"O",  ACT_APPEND}, {"P", ACT_APPEND}, {"Q", ACT_APPEND}, {"R", ACT_APPEND}, {"S", ACT_APPEND}, {"T",  ACT_APPEND}, {"U", ACT_APPEND}, 
   {"V",  ACT_APPEND}, {"W", ACT_APPEND}, {"X", ACT_APPEND}, {"Y", ACT_APPEND}, {"Z", ACT_APPEND}, {"a",  ACT_APPEND}, {"b", ACT_APPEND}, 
   {"c",  ACT_APPEND}, {"d", ACT_APPEND}, {"e", ACT_APPEND}, {"f", ACT_APPEND}, {"g", ACT_APPEND}, {"h",  ACT_APPEND}, {"i", ACT_APPEND}, 
   {"j",  ACT_APPEND}, {"k", ACT_APPEND}, {"l", ACT_APPEND}, {"m", ACT_APPEND}, {"n", ACT_APPEND}, {"o",  ACT_APPEND}, {"p", ACT_APPEND}, 
   {"q",  ACT_APPEND}, {"r", ACT_APPEND}, {"s", ACT_APPEND}, {"t", ACT_APPEND}, {"u", ACT_APPEND}, {"v",  ACT_APPEND}, {"w", ACT_APPEND}, 
   {"x",  ACT_APPEND}, {"y", ACT_APPEND}, {"z", ACT_APPEND}, {"0", ACT_APPEND}, {"1", ACT_APPEND}, {"2",  ACT_APPEND}, {"3", ACT_APPEND}, 
   {"4",  ACT_APPEND}, {"5", ACT_APPEND}, {"6", ACT_APPEND}, {"7", ACT_APPEND}, {"8", ACT_APPEND}, {"9",  ACT_APPEND}, {"-", ACT_APPEND}, 
   {"_",  ACT_APPEND}, {"@", ACT_APPEND}, {"#", ACT_APPEND}, {",", ACT_APPEND}, {".", ACT_APPEND}, {"?",  ACT_APPEND}, {"/", ACT_APPEND}, 
   {"~",  ACT_APPEND}, {"!", ACT_APPEND}, {"$", ACT_APPEND}, {"%", ACT_APPEND}, {"^", ACT_APPEND}, {"&",  ACT_APPEND}, {"*", ACT_APPEND}, 
   {"\\", ACT_APPEND}, {"(", ACT_APPEND}, {")", ACT_APPEND}, {"+", ACT_APPEND}, {"`", ACT_APPEND}, {"'",  ACT_APPEND}, {";", ACT_APPEND}, 
   {"<",  ACT_APPEND}, {">", ACT_APPEND}, {"|", ACT_APPEND}, {"=", ACT_APPEND}, {"~", ACT_APPEND}, {"\"", ACT_APPEND}, {":", ACT_APPEND}, 
  };

PanelCell CellsUcaseLetters[] =
  {
   {"A", ACT_APPEND}, {"B", ACT_APPEND}, {"C", ACT_APPEND}, {"D", ACT_APPEND}, {"E", ACT_APPEND}, {"F", ACT_APPEND}, {"G", ACT_APPEND},
   {"H", ACT_APPEND}, {"I", ACT_APPEND}, {"J", ACT_APPEND}, {"K", ACT_APPEND}, {"L", ACT_APPEND}, {"M", ACT_APPEND}, {"N", ACT_APPEND},
   {"O", ACT_APPEND}, {"P", ACT_APPEND}, {"Q", ACT_APPEND}, {"R", ACT_APPEND}, {"S", ACT_APPEND}, {"T", ACT_APPEND}, {"U", ACT_APPEND},
   {"V", ACT_APPEND}, {"W", ACT_APPEND}, {"X", ACT_APPEND}, {"Y", ACT_APPEND}, {"Z", ACT_APPEND}, {"-", ACT_APPEND}, {"_", ACT_APPEND},
  };

PanelCell CellsLcaseLetters[] =
  {
   {"a", ACT_APPEND}, {"b", ACT_APPEND}, {"c", ACT_APPEND}, {"d", ACT_APPEND}, {"e", ACT_APPEND}, {"f", ACT_APPEND}, {"g", ACT_APPEND},
   {"h", ACT_APPEND}, {"i", ACT_APPEND}, {"j", ACT_APPEND}, {"k", ACT_APPEND}, {"l", ACT_APPEND}, {"m", ACT_APPEND}, {"n", ACT_APPEND},
   {"o", ACT_APPEND}, {"p", ACT_APPEND}, {"q", ACT_APPEND}, {"r", ACT_APPEND}, {"s", ACT_APPEND}, {"t", ACT_APPEND}, {"u", ACT_APPEND},
   {"v", ACT_APPEND}, {"w", ACT_APPEND}, {"x", ACT_APPEND}, {"y", ACT_APPEND}, {"z", ACT_APPEND}, {"-", ACT_APPEND}, {"_", ACT_APPEND},
  };

PanelCell CellsNumbersAndSpecialChars[] =
  {
   {"1", ACT_APPEND}, {"2", ACT_APPEND}, {"3", ACT_APPEND}, {"4", ACT_APPEND}, {"5", ACT_APPEND}, {"6", ACT_APPEND}, {"7", ACT_APPEND},
   {"8", ACT_APPEND}, {"9", ACT_APPEND}, {"0", ACT_APPEND}, {"-", ACT_APPEND}, {"_", ACT_APPEND}, {",", ACT_APPEND}, {".", ACT_APPEND},
   {"?", ACT_APPEND}, {"/", ACT_APPEND}, {"~", ACT_APPEND}, {"#", ACT_APPEND}, {"@", ACT_APPEND}, {"-", ACT_APPEND}, {"_", ACT_APPEND},
  };

PanelCell CellsRemainingSpecialChars[] =
  {
   {"!", ACT_APPEND}, {"@", ACT_APPEND},  {"#", ACT_APPEND}, {"$",  ACT_APPEND}, {"%", ACT_APPEND}, {"^", ACT_APPEND}, {"&", ACT_APPEND},
   {"*", ACT_APPEND}, {"\\", ACT_APPEND}, {"(", ACT_APPEND}, {")",  ACT_APPEND}, {"{", ACT_APPEND}, {"}", ACT_APPEND}, {"+", ACT_APPEND},
   {"`", ACT_APPEND}, {"'",  ACT_APPEND}, {";", ACT_APPEND}, {"<",  ACT_APPEND}, {">", ACT_APPEND}, {"[", ACT_APPEND}, {"]", ACT_APPEND},
   {"|", ACT_APPEND}, {"=", ACT_APPEND},  {"~", ACT_APPEND}, {"\"", ACT_APPEND}, {":", ACT_APPEND}, {"-", ACT_APPEND}, {"_", ACT_APPEND},
  };

PanelCell CellsWifiSelect[] =
  {
   {"Ctrl-45-Reg", ACT_DONE}, 
   {"Ctrl-45-Inv", ACT_DONE}, 
   {"Ctrl-Horz",   ACT_DONE}, 
   {"AnkiRobits",  ACT_DONE}, 
   {"Anki5Ghz",    ACT_DONE}, 
   {"AnkiGuest",   ACT_DONE}, 
   {"BeagleBone",  ACT_DONE}, 
   {"SFWireless",  ACT_DONE}, 
   {"wireless",    ACT_DONE}, 
   {"wireless-2",  ACT_DONE}, 
   {"wireless-3",  ACT_DONE}, 
  };

//                                    r   c  PanelCell[]                  IsSelectMenu
Panel kPanelAllChars               = {13, 7, CellsAllChars,               false}; 
Panel kPanelUcaseLetters           = {4,  7, CellsUcaseLetters,           false}; 
Panel kPanelLcaseLetters           = {4,  7, CellsLcaseLetters,           false}; 
Panel kPanelNumbersAndSpecialChars = {3,  7, CellsNumbersAndSpecialChars, false}; 
Panel kPanelRemainingSpecialChars  = {4,  7, CellsRemainingSpecialChars,  false}; 
Panel kPanelWifiSelect             = {11, 1, CellsWifiSelect,             true};  


Panel* PasswordEntryPanels[] =
  {
   &kPanelAllChars,
   &kPanelUcaseLetters,
   &kPanelLcaseLetters,
   &kPanelNumbersAndSpecialChars,
   &kPanelRemainingSpecialChars,
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
void BehaviorCubeDrive::DisplayHeaderText(string text, ColorRGBA color, float maxWidth)
{
  _dVars.image = Vision::Image(FACE_DISPLAY_HEIGHT,FACE_DISPLAY_WIDTH, NamedColors::BLACK);

  float scale    = kUserTextScaleFullSize;
  Vec2f textSize = _dVars.image.GetTextSize(text, scale, 1);
  if (textSize.x() > FACE_DISPLAY_WIDTH) {
    scale = kUserTextScaleMidSize;
  }
  textSize = _dVars.image.GetTextSize(text, scale, 1);
  if (textSize.x() > maxWidth) {
    text     = ".." + text;
    textSize = _dVars.image.GetTextSize(text, scale, 1);
    while (textSize.x() > maxWidth) {
      text     = ".." + text.substr(3, string::npos);
      textSize = _dVars.image.GetTextSize(text, scale, 1);
    }
  }
  _dVars.image.DrawText(Point2f(0, kTopLeftCornerMagicNumber),
                        text, NamedColors::WHITE, scale);
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

  _controlScheme  = CTRL_45_REG;
  _panelSet       = &WifiSelect;
  _currPanel      = 0;
  _promptText     = kWifiSelectPrompt;
  _userText       = "";
  _row            = 0;
  _col            = 0;
  _firstScreenRow = 0;

  ClearHoldCounts();
  _deadZoneTicksLeft = 0;
  for(int i = 0; i < MAX_DIR_COUNT; i++) {
    _dirCountEventMap[i] = false;
  }
  _dirCountEventMap[2] = true;  // first
  // delay
  for (int i = 0; i < 30; i++) {
    _dirCountEventMap[10 + (4*i)] = true; // repeat
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

    float xGs = GetBEI().GetRobotInfo().GetHeadAccelData().x / 9810.0;
    float yGs = GetBEI().GetRobotInfo().GetHeadAccelData().y / 9810.0;
    float zGs = GetBEI().GetRobotInfo().GetHeadAccelData().z / 9810.0;
    // At most, only one direction can be active on any tick
    int dir = NUM_DIR;  // none
    if (_controlScheme != CTRL_HORZ) {
      xGs -= kAccelOffsetX;
      if (abs(xGs) > abs(yGs)) {
        yGs = 0.0;
      } else {
        xGs = 0.0;
      }
      if ((yGs < (-kAccelThresh)) && (abs(xGs) < kAccelThresh)) { dir = DIR_R; }
      if ((yGs > (+kAccelThresh)) && (abs(xGs) < kAccelThresh)) { dir = DIR_L; }
      if (_controlScheme == CTRL_45_INV) {
        if ((xGs < (-kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_U; }
        if ((xGs > (+kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_D; }
      } else {
        if ((xGs < (-kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_D; }
        if ((xGs > (+kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_U; }
      }
    } else {  // _controlScheme == CTRL_HORZ
      if (abs(zGs) > abs(yGs)) {
        yGs = 0.0;
      } else {
        zGs = 0.0;
      }
      if ((yGs < (-kAccelThresh)) && (abs(zGs) < kAccelThresh)) { dir = DIR_R; }
      if ((yGs > (+kAccelThresh)) && (abs(zGs) < kAccelThresh)) { dir = DIR_L; }
      if ((zGs < (-kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_U; }
      if ((zGs > (+kAccelThresh)) && (abs(yGs) < kAccelThresh)) { dir = DIR_D; }
    }

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
        NewDirEvent(dir, panel->NumRows-1, panel->NumCols);
        // Note: (col == NumCols) is for "nav column" on right-hand-side
      }
    }

    //if(GetBEI().GetCubeInteractionTracker().GetTargetStatus().tappedDuringLastTick) {
    if (newButtonEvent) {
      string text   = "";   
      int    action = ACT_NOP; 
      if (_col < panel->NumCols) {
        PanelCell pc = GetPanelCell(panel, _row, _col);
        text   = pc.Text;
        action = pc.Action;
      } else if (!panel->IsSelectMenu) {
        switch (_row - _firstScreenRow) {
        case 0:
          _row = (_row <= 0) ? 0 : (_row - 1);
          break;
        case 1:
          action = ACT_DELETE;
          break;
        case 2:
          action = ACT_DONE;
          break;
        case 3:
          _row = (_row >= (panel->NumRows-1)) ? (panel->NumRows-1) : (_row + 1);
          break;
        }
      }

      switch (action) {
      case ACT_APPEND:
        _userText += text;
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
      case ACT_DONE:
        // XXX: lightweight on-boarding flow = toggle between Wifi Select and Password Entry
        if (_promptText != kPasswordEntryPrompt) {
          if (_row < NUM_CTRL) {
            // XXX: Terrible hack to change control scheme while running
            _controlScheme = _row;
          }
          _panelSet      = &PasswordEntry;
          _promptText    = kPasswordEntryPrompt;
        } else {
          _panelSet      = &WifiSelect;
          _promptText    = kWifiSelectPrompt;
        }
        _userText       = "";
        _row            = 0;
        _col            = 0;
        _currPanel      = 0;
        _firstScreenRow = 0;
        break;
      }
    }

    // Update the screen: heading
    string heading = (_userText != "") ? _userText : _promptText;
    DisplayHeaderText(heading, NamedColors::WHITE, FACE_DISPLAY_WIDTH);

    // Update the screen: select text
    if (_row < _firstScreenRow) {
      _firstScreenRow = _row;
    } else if (_row >= (_firstScreenRow+kNumSelectRows)) {
      _firstScreenRow = _row - kNumSelectRows + 1;
    }
    for(int roffset = 0; roffset < kNumSelectRows; roffset++) {
      int r = _firstScreenRow + roffset;
      if (r >= panel->NumRows) {
        continue;
      }
      for (int c = 0; c < panel->NumCols; c++) {
        Point2f p = Point2f((float(c)         * kTextHorzSpace),
                            (float(roffset+1) * kTextVertSpace) + kSelectRowStart);
        string t = GetPanelCell(panel, r, c).Text;
        ColorRGBA color(0.4f, 0.4f, 0.4f);
        if ((r == _row) && (c == _col)) {
          color = NamedColors::WHITE;
        }
        _dVars.image.DrawText(p, t, color, kSelectTextScale);
      }
    }

    // Add a navigation column on the right side
    for(int roffset = 0; roffset < 4; roffset++) {
      int r = _firstScreenRow + roffset;
      int c = 7;
      string t = " .";
      switch (roffset) {
      case 0: if (_firstScreenRow > 0)                                 { t = "/\\"; } else { t = "--"; } break;
      case 1: if (!panel->IsSelectMenu)                                { t = "Del"; } else { t = " |"; } break;
      case 2: if (!panel->IsSelectMenu)                                { t = "OK";  } else { t = " |"; } break;
      case 3: if ((_firstScreenRow + kNumSelectRows) < panel->NumRows) { t = "\\/"; } else { t = "--"; } break;
      }
      Point2f p = Point2f((float(c)         * kTextHorzSpace),
                          (float(roffset+1) * kTextVertSpace) + kSelectRowStart);

      ColorRGBA color = NamedColors::RED;  // XXX: It's totally NOT red
      if ((r == _row) && (_col == panel->NumCols)) {
        color = NamedColors::WHITE;
      }
      _dVars.image.DrawText(p, t, color, kSelectTextScale);
    }

    GetBEI().GetAnimationComponent().DisplayFaceImage(_dVars.image, 1.0f, true);
  }
}

} // namespace Vector
} // namespace Anki
