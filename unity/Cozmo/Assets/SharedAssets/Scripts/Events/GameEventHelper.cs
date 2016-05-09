using System;
using Anki.Cozmo;
using System.Collections.Generic;
using System.Reflection;
using System.Linq;

// Saves the GameEvents as a string so they don't get out of order when they serielize
[Serializable]
public class SerializableGameEvents : SerializableEnum<Anki.Cozmo.GameEvent> {
  // Init as invalid
  public SerializableGameEvents() {
    Value = GameEvent.Count;
  }
}

/// <summary>
/// Individual Events that contain optional parameters.
/// CLAD doesn't support inheritence, so keeping these in C#
/// </summary>
public class GameEventWrapper {
  public GameEvent GameEventEnum { get; set; }
  
  // So this is easy to call for other classes, just pass along variable length arguments.
  // Trying to pass in non-exact matches to Activator.CreateInstance causes runtime explosions.
  // and Dictionaries can get to verbose.
  public virtual void Init(GameEvent Enum, params object[] args) {
    GameEventEnum = Enum;
  }
}

public class MinigameCompletedGameEvent : GameEventWrapper {
  public int Difficulty;
  public string GameID;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);
    if (args.Length > 0 && args[0].GetType() == typeof(Int32)) {
      Difficulty = (int)args[0];
    }
    // Potentially this could also attempt to guess the ID by the enum type.
    // Just depends on what the event is.
    // Not needed if the event name includes the game type.
    if (args.Length > 1 && args[1].GetType() == typeof(String)) {
      GameID = (string)args[1];
    }
  }
}


/// <summary>
///  Factory to create helper events so nothing has to think about them.
/// </summary>
public class GameEventWrapperFactory {
  public static GameEventWrapper Create(Anki.Cozmo.GameEvent Enum, params object[] args) {
    // Return base class if no specialized version was Registered in Init.
    Type type = typeof(GameEventWrapper);
    if (!_EnumWrappers.TryGetValue(Enum, out type)) {
      type = typeof(GameEventWrapper);
    }

    // C# doesn't have perfect forwarding in constructors so do any fancy work that requires parameters in "init"
    GameEventWrapper out_ev = (GameEventWrapper)System.Activator.CreateInstance(type);
    out_ev.Init(Enum, args);
    return out_ev;
  }

  private static Dictionary<Anki.Cozmo.GameEvent,Type> _EnumWrappers;

  public static void Init() {
    _EnumWrappers = new Dictionary<Anki.Cozmo.GameEvent,Type>();
    // Specific wrappers, otherwise just use the base class.
    Register(GameEvent.OnSpeedtapGameCozmoWin, typeof(MinigameCompletedGameEvent));
    Register(GameEvent.OnSpeedtapGamePlayerWin, typeof(MinigameCompletedGameEvent));
  }

  private static void Register(Anki.Cozmo.GameEvent Enum, Type type) {
    _EnumWrappers.Add(Enum, type);
  }
}
