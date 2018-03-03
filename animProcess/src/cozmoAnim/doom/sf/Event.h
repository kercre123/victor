#pragma once

// namespace is sf to avoid renaming all types. this is a vestige from SFML
namespace sf {

class Event
{
public:
  
  Event(){}
  
  enum EventType
  {
//      Closed,
//      Resized,
//      LostFocus,
//      GainedFocus,
      TextEntered,
      KeyPressed,
      KeyReleased,
//      MouseWheelMoved,
//      MouseWheelScrolled,
//      MouseButtonPressed,
//      MouseButtonReleased,
//      MouseMoved,
//      MouseEntered,
//      MouseLeft,
//      JoystickButtonPressed,
//      JoystickButtonReleased,
//      JoystickMoved,
//      JoystickConnected,
//      JoystickDisconnected,
//      TouchBegan,
//      TouchMoved,
//      TouchEnded,
//      SensorChanged,
//
      Count
  };
  
  enum Key
  {
    Unknown = -1,
    A = 0,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Escape,
    LControl,
    LShift,
    LAlt,
    LSystem,
    RControl,
    RShift,
    RAlt,
    RSystem,
    Menu,
    LBracket,
    RBracket,
    SemiColon,
    Comma,
    Period,
    Quote,
    Slash,
    BackSlash,
    Tilde,
    Equal,
    Dash,
    Space,
    Return,
    BackSpace,
    Tab,
    PageUp,
    PageDown,
    End,
    Home,
    Insert,
    Delete,
    Add,
    Subtract,
    Multiply,
    Divide,
    Left,
    Right,
    Up,
    Down,
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    Pause,

    KeyCount
  };
  
  struct KeyEvent
  {
    Key           code;
    bool          alt;
    bool          control;
    bool          shift;
    bool          system;
  };
  
  struct TextEvent
  {
    unsigned int unicode;
  };
  
  EventType type;
  
  union
  {
//       SizeEvent             size;
       KeyEvent              key;
       TextEvent             text;
//       MouseMoveEvent        mouseMove;
//       MouseButtonEvent      mouseButton;
//       MouseWheelEvent       mouseWheel;
//       MouseWheelScrollEvent mouseWheelScroll;
//       JoystickMoveEvent     joystickMove;
//       JoystickButtonEvent   joystickButton;
//       JoystickConnectEvent  joystickConnect;
//       TouchEvent            touch;
//       SensorEvent           sensor;
  };
};
  
}
