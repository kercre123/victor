
#include "cozmoAnim/doom/doomPort.h"


#include "cozmoAnim/doom/doomdef.h"
#include "cozmoAnim/doom/doomAudioMixer.h"

#include "cozmoAnim/doom/m_argv.h"
#include "cozmoAnim/doom/d_main.h"
#include "cozmoAnim/doom/sf/Graphics.h"
#include "cozmoAnim/doom/sf/Event.h"
#include "cozmoAnim/doom/sf/Audio.h"


#include "coretech/vision/engine/image.h"
#include "coretech/common/engine/math/matrix_impl.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/math/numericCast.h"

#include "clad/robotInterface/messageRobotToEngine.h"

#include "util/console/consoleInterface.h"

#include <iostream>

DoomPort::DoomPort(const std::string& resourcePath, unsigned int width, unsigned int height)
  : _thread()
{
  _gDoomResourcePath = resourcePath;
  if( _gDoomResourcePath.back() != '/' ) {
    _gDoomResourcePath += "/";
  }
  
  
  // todo: use ctor params to init cmdline params
  CmdParameters(0, NULL);
  
  _gDoomPortWidth = width;
  _gDoomPortHeight = height;
  
}

void DoomPort::SetAudioController( Anki::Cozmo::Audio::CozmoAudioController* ac )
{
  _gMixer.SetAudioController( ac );
}

void DoomPort::Run() {
  _thread = std::thread{&DoomPort::CallDoomMain, this};
}

void DoomPort::Stop() {
  _gDoomPortRun = false;
  _thread.join();
}

void DoomPort::CallDoomMain() {
  try {
    D_DoomMain();
  } catch (std::exception ex) {
    std::lock_guard<std::mutex> lock(_mutex);
    _exception = std::current_exception();
  }
}

void DoomPort::Update()
{
  _gMixer.FlushPlayQueue();
}

std::exception_ptr DoomPort::GetException()
{
  std::exception_ptr ret;
  {
    std::lock_guard<std::mutex> lock(_mutex);
    ret = _exception;
  }
  return ret;
}

void DoomPort::GetScreen(Anki::Vision::ImageRGB565& screen)
{
  if( window != nullptr ) {
    window->GetScreen(screen);
  } else {
    PRINT_NAMED_WARNING("DOOM","no window yet");
    // Display three color strips increasing in brightness from left to right
    for(int i=0; i<Anki::Cozmo::FACE_DISPLAY_HEIGHT/3; ++i)
    {
      Anki::Vision::PixelRGB565* red_i   = screen.GetRow(i);
      Anki::Vision::PixelRGB565* green_i = screen.GetRow(i + Anki::Cozmo::FACE_DISPLAY_HEIGHT/3);
      Anki::Vision::PixelRGB565* blue_i  = screen.GetRow(i + 2*Anki::Cozmo::FACE_DISPLAY_HEIGHT/3);
      for(int j=0; j<Anki::Cozmo::FACE_DISPLAY_WIDTH; ++j)
      {
        const u8 value = Anki::Util::numeric_cast_clamped<u8>(std::round((f32)j/(f32)Anki::Cozmo::FACE_DISPLAY_WIDTH * 255.f));
        red_i[j]   = Anki::Vision::PixelRGB565(value, 0, 0);
        green_i[j] = Anki::Vision::PixelRGB565(0, value, 0);
        blue_i[j]  = Anki::Vision::PixelRGB565(0, 0, value);
      }
    }
  }
}

void InputFire()
{
  sf::Event ev;
  ev.type = sf::Event::KeyPressed;
  ev.key.code = sf::Event::Key::LControl; // attack
  window->AddEvent( std::move(ev) );
}
  
void InputFireRelease()
{
  sf::Event ev;
  ev.type = sf::Event::KeyReleased;
  ev.key.code = sf::Event::Key::LControl;
  window->AddEvent( std::move(ev) );
}

void InputUse()
{
  sf::Event ev;
  ev.type = sf::Event::KeyPressed;
  ev.key.code = sf::Event::Key::Space; // use
  window->AddEvent( std::move(ev) );
}
void InputUseRelease()
{
  sf::Event ev;
  ev.type = sf::Event::KeyReleased;
  ev.key.code = sf::Event::Key::Space; // use
  window->AddEvent( std::move(ev) );
}

void InputForward()
{
  sf::Event ev;
  ev.type = sf::Event::KeyPressed;
  ev.key.code = sf::Event::Key::W;
  window->AddEvent( std::move(ev) );
}
void InputForwardRelease()
{
  sf::Event ev;
  ev.type = sf::Event::KeyReleased;
  ev.key.code = sf::Event::Key::W;
  window->AddEvent( std::move(ev) );
}

void InputBackward()
{
  sf::Event ev;
  ev.type = sf::Event::KeyPressed;
  ev.key.code = sf::Event::Key::S;
  window->AddEvent( std::move(ev) );
}
void InputBackwardRelease()
{
  sf::Event ev;
  ev.type = sf::Event::KeyReleased;
  ev.key.code = sf::Event::Key::S;
  window->AddEvent( std::move(ev) );
}

void InputLeft()
{
  sf::Event ev;
  ev.type = sf::Event::KeyPressed;
  ev.key.code = sf::Event::Key::Left;
  window->AddEvent( std::move(ev) );
}
void InputLeftRelease()
{
  sf::Event ev;
  ev.type = sf::Event::KeyReleased;
  ev.key.code = sf::Event::Key::Left;
  window->AddEvent( std::move(ev) );
}

void InputRight()
{
  sf::Event ev;
  ev.type = sf::Event::KeyPressed;
  ev.key.code = sf::Event::Key::Right;
  window->AddEvent( std::move(ev) );
}
void InputRightRelease()
{
  sf::Event ev;
  ev.type = sf::Event::KeyReleased;
  ev.key.code = sf::Event::Key::Right;
  window->AddEvent( std::move(ev) );
}

void ConsoleInputFire( ConsoleFunctionContextRef context ) { InputFire(); }
CONSOLE_FUNC(ConsoleInputFire, "AAA DOOM");
void ConsoleInputFireRelease( ConsoleFunctionContextRef context ) { InputFireRelease(); }
CONSOLE_FUNC(ConsoleInputFireRelease, "AAA DOOM");
void ConsoleInputUse( ConsoleFunctionContextRef context ) { InputUse(); }
CONSOLE_FUNC(ConsoleInputUse, "AAA DOOM");
void ConsoleInputUseRelease( ConsoleFunctionContextRef context ) { InputUseRelease(); }
CONSOLE_FUNC(ConsoleInputUseRelease, "AAA DOOM");

void ConsoleInputForward( ConsoleFunctionContextRef context ) { InputForward(); }
CONSOLE_FUNC(ConsoleInputForward, "AAA DOOM");
void ConsoleInputForwardRelease( ConsoleFunctionContextRef context ) { InputForwardRelease(); }
CONSOLE_FUNC(ConsoleInputForwardRelease, "AAA DOOM");

void ConsoleInputBackward( ConsoleFunctionContextRef context ) { InputBackward(); }
CONSOLE_FUNC(ConsoleInputBackward, "AAA DOOM");
void ConsoleInputBackwardRelease( ConsoleFunctionContextRef context ) { InputBackwardRelease(); }
CONSOLE_FUNC(ConsoleInputBackwardRelease, "AAA DOOM");

void ConsoleInputLeft( ConsoleFunctionContextRef context ) { InputLeft(); }
CONSOLE_FUNC(ConsoleInputLeft, "AAA DOOM");
void ConsoleInputLeftRelease( ConsoleFunctionContextRef context ) { InputLeftRelease(); }
CONSOLE_FUNC(ConsoleInputLeftRelease, "AAA DOOM");

void ConsoleInputRight( ConsoleFunctionContextRef context ) { InputRight(); }
CONSOLE_FUNC(ConsoleInputRight, "AAA DOOM");
void ConsoleInputRightRelease( ConsoleFunctionContextRef context ) { InputRightRelease(); }
CONSOLE_FUNC(ConsoleInputRightRelease, "AAA DOOM");


void DoomPort::HandleMessage(const Anki::Cozmo::RobotState& robotState)
{
  if( !_gMainLoopStarted ) {
    return;
  }
  
  // todo: helper for this shit
  static bool buttonWasPressed = false;
  const auto buttonIsPressed = static_cast<bool>(robotState.status & (uint16_t)Anki::Cozmo::RobotStatusFlag::IS_BUTTON_PRESSED);
  const auto buttonPressedEvent = !buttonWasPressed && buttonIsPressed;
  const auto buttonReleasedEvent = buttonWasPressed && !buttonIsPressed;
  buttonWasPressed = buttonIsPressed;
  
  const auto liftHeight_mm = ((sinf(robotState.liftAngle) * Anki::Cozmo::LIFT_ARM_LENGTH) + Anki::Cozmo::LIFT_BASE_POSITION[2] + Anki::Cozmo::LIFT_FORK_HEIGHT_REL_TO_ARM_END);
  static bool wasLiftAbove = false;
  const bool isAbove = liftHeight_mm > 50.0;
  const bool useButtonEvent = !wasLiftAbove && isAbove;
  const bool useButtonReleasedEvent = wasLiftAbove && !isAbove;
  wasLiftAbove = isAbove;
  
  const bool rockedBack = (robotState.pose.pitch_angle > DEG_TO_RAD(20.0f));
  const bool rockedForward = (robotState.pose.pitch_angle < -DEG_TO_RAD(10.0f)); // smaller than back
  static bool wasBack = false;
  static bool wasForward = false;
  bool moveForwardEvent = !wasBack && rockedBack;
  bool moveForwardReleaseEvent = wasBack && !rockedBack;
  bool moveBackwardEvent = !wasForward && rockedForward;
  bool moveBackwardReleaseEvent = wasForward && !rockedForward;
  wasBack = rockedBack;
  wasForward = rockedForward;
  
  const float kAccelFilterConstant = 0.50f;
  _robotAccelYFiltered = (kAccelFilterConstant * _robotAccelYFiltered)
                          + ((1.0f - kAccelFilterConstant) * robotState.accel.y);
  const float kAccel = 2000.0f; // compare to 9800
  const bool rockedRight = _robotAccelYFiltered < -kAccel;
  const bool rockedLeft = _robotAccelYFiltered > kAccel;
  static bool wasRight = false;
  static bool wasLeft = false;
  bool moveLeftEvent = !wasLeft && rockedLeft;
  bool moveLeftReleaseEvent = wasLeft && !rockedLeft;
  bool moveRightEvent = !wasRight && rockedRight;
  bool moveRightReleaseEvent = wasRight && !rockedRight;
  wasRight = rockedRight;
  wasLeft = rockedLeft;
  
//  std::stringstream ss;
//
//  ss << "pose: " << robotState.pose.x << ", " << robotState.pose.y << ", " << robotState.pose.z
//     << ", " << robotState.pose.angle << ", " << robotState.pose.pitch_angle << std::endl;
//  ss << "headAngle: " << robotState.headAngle << std::endl;
//  ss << "liftHeight_mm: " << liftHeight_mm << std::endl;
//  ss << "accel: " << robotState.accel.x << ", " << robotState.accel.y << ", " << robotState.accel.z << std::endl;
//  ss << "gyro: " << robotState.gyro.x << ", " << robotState.gyro.y << ", " << robotState.gyro.z << std::endl;
//  ss << "button: " << buttonPressedEvent << std::endl;

//  PRINT_NAMED_WARNING("DOOM", "lift: %f %d", liftHeight_mm, useButtonEvent);
//  PRINT_NAMED_WARNING("DOOM", "%s",ss.str().c_str());
//  PRINT_NAMED_WARNING("DOOM", "%d %d %d %d %d %d %d %d %d %d %d %d",
//                      buttonPressedEvent,buttonReleasedEvent,useButtonEvent,useButtonReleasedEvent,
//                      moveForwardEvent,moveForwardReleaseEvent, moveBackwardEvent, moveBackwardReleaseEvent,
//                      moveLeftEvent, moveLeftReleaseEvent, moveRightEvent,moveRightReleaseEvent);
  

  if( buttonPressedEvent ) {
    InputFire();
  }
  if( buttonReleasedEvent ) {
    InputFireRelease();
  }
  
  if( useButtonEvent ) {
    PRINT_NAMED_WARNING("DOOM", "lift press: %f", liftHeight_mm);
    InputUse();
  }
  if( useButtonReleasedEvent ) {
    PRINT_NAMED_WARNING("DOOM", "lift release: %f", liftHeight_mm);
    InputUseRelease();
  }
  
  if( moveForwardEvent ) {
    PRINT_NAMED_WARNING("DOOM", "forward press: %f", robotState.pose.pitch_angle);
    InputForward();
  }
  if( moveForwardReleaseEvent ) {
    PRINT_NAMED_WARNING("DOOM", "forward release: %f", robotState.pose.pitch_angle);
    InputForwardRelease();
  }
  
  if( moveBackwardEvent ) {
    PRINT_NAMED_WARNING("DOOM", "back press: %f", robotState.pose.pitch_angle);
    InputBackward();
  }
  if( moveBackwardReleaseEvent ) {
    PRINT_NAMED_WARNING("DOOM", "back release: %f", robotState.pose.pitch_angle);
    InputBackwardRelease();
  }
  
  if( moveLeftEvent ) {
    PRINT_NAMED_WARNING("DOOM", "left press: %f", _robotAccelYFiltered);
    InputLeft();
  }
  if( moveLeftReleaseEvent ) {
    PRINT_NAMED_WARNING("DOOM", "left release: %f", _robotAccelYFiltered);
    InputLeftRelease();
  }
  
  if( moveRightEvent ) {
    PRINT_NAMED_WARNING("DOOM", "right press: %f", _robotAccelYFiltered);
    InputRight();
  }
  if( moveRightReleaseEvent ) {
    PRINT_NAMED_WARNING("DOOM", "right release: %f", _robotAccelYFiltered);
    InputRightRelease();
  }
  
  
  // unused but could be useful:
  //Anki::Cozmo::ProxSensorData proxData;
  // uint16_t backpackTouchSensorRaw;
  
}
