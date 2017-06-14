using System;
using Anki.Cozmo;
using System.Collections.Generic;
using System.Reflection;
using System.Linq;
using Cozmo.UI;

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

public class ChallengeStartedGameEvent : GameEventWrapper {
  public string GameID;
  public int Difficulty;
  public bool RequestedGame;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);
    if (args.Length > 0 && args[0].GetType() == typeof(String)) {
      GameID = (string)args[0];
    }

    if (args.Length > 1 && args[1].GetType() == typeof(bool)) {
      RequestedGame = (bool)args[1];
    }

  }
}

public class ChallengeGameEvent : GameEventWrapper {
  public string GameID;
  public int Difficulty;
  // Winner refers to different things in different events. But will always be true
  // if the player has "won" whichever event it is. Examples include...
  // Hand - Whoever just scored a point
  // Round - Whoever won the round
  // Game - Whoever won the entire game
  public bool PlayerWin;
  public int PlayerScore;
  public int CozmoScore;
  public bool HighIntensity;
  public Dictionary<string, float> GameSpecificValues;

  /// <summary>
  /// Args in Order....
  /// string GameID
  /// int CurrentDifficulty
  /// bool PlayerWin
  /// int CurrentPlayerScore
  /// int CurrentCozmoScore
  /// bool HighIntensity
  /// </summary>
  /// <param name="Enum">Enum.</param>
  /// <param name="args">Arguments.</param>
  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);

    if (args.Length > 0 && args[0].GetType() == typeof(String)) {
      GameID = (string)args[0];
    }

    if (args.Length > 1 && args[1].GetType() == typeof(Int32)) {
      Difficulty = (int)args[1];
    }

    if (args.Length > 2 && args[2].GetType() == typeof(bool)) {
      PlayerWin = (bool)args[2];
    }

    if (args.Length > 3 && args[3].GetType() == typeof(Int32)) {
      PlayerScore = (int)args[3];
    }

    if (args.Length > 4 && args[4].GetType() == typeof(Int32)) {
      CozmoScore = (int)args[4];
    }

    if (args.Length > 5 && args[5].GetType() == typeof(bool)) {
      HighIntensity = (bool)args[5];
    }
    // Things like Lives in MemoryMatch...
    if (args.Length > 6 && args[6] != null && args[6].GetType() == typeof(Dictionary<string, float>)) {
      GameSpecificValues = (Dictionary<string, float>)args[6];
    }
  }

}

public class NewHighScoreGameEvent : ChallengeGameEvent {
  public int OldHighScore;
  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum, args);
    if (args.Length > 7 && args[7].GetType() == typeof(int)) {
      OldHighScore = (int)args[7];
    }
  }
}

public class DifficultyUnlockedGameEvent : GameEventWrapper {
  public string GameID;
  public int NewDifficulty;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);
    if (args.Length > 0 && args[0].GetType() == typeof(String)) {
      GameID = (string)args[0];
    }
    if (args.Length > 1 && args[1].GetType() == typeof(Int32)) {
      NewDifficulty = (int)args[1];
    }
  }
}

public class UnlockableGameEvent : GameEventWrapper {
  public UnlockId Unlock;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);
    if (args.Length > 0 && args[0].GetType() == typeof(UnlockId)) {
      Unlock = (UnlockId)args[0];
    }
  }

}

public class SessionStartedGameEvent : GameEventWrapper {
  public int CurrentStreak;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);
    if (args.Length > 0 && args[0].GetType() == typeof(int)) {
      CurrentStreak = (int)args[0];
    }
  }
}

public class BehaviorSuccessGameEvent : GameEventWrapper {
  public BehaviorObjective Behavior;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);
    if (args.Length > 0 && args[0].GetType() == typeof(BehaviorObjective)) {
      Behavior = (BehaviorObjective)args[0];
    }
  }
}

public class TimedIntervalGameEvent : GameEventWrapper {
  public float TargetTime_Sec;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);
    if (args.Length > 0 && args[0].GetType() == typeof(float)) {
      TargetTime_Sec = (float)args[0];
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

  private static Dictionary<Anki.Cozmo.GameEvent, Type> _EnumWrappers;

  public static void Init() {
    _EnumWrappers = new Dictionary<Anki.Cozmo.GameEvent, Type>();
    // Specific wrappers, otherwise just use the base class.
    Register(GameEvent.OnChallengeStarted, typeof(ChallengeStartedGameEvent));
    Register(GameEvent.OnChallengePointScored, typeof(ChallengeGameEvent));
    Register(GameEvent.OnChallengeRoundEnd, typeof(ChallengeGameEvent));
    Register(GameEvent.OnChallengeComplete, typeof(ChallengeGameEvent));
    Register(GameEvent.OnChallengeDifficultyUnlock, typeof(DifficultyUnlockedGameEvent));
    Register(GameEvent.OnUnlockableEarned, typeof(UnlockableGameEvent));
    Register(GameEvent.OnNewDayStarted, typeof(SessionStartedGameEvent));
    Register(GameEvent.OnUnlockableSparked, typeof(UnlockableGameEvent));
    Register(GameEvent.OnFreeplayBehaviorSuccess, typeof(BehaviorSuccessGameEvent));
    Register(GameEvent.OnFreeplayInterval, typeof(TimedIntervalGameEvent));
    Register(GameEvent.OnChallengeInterval, typeof(TimedIntervalGameEvent));
    Register(GameEvent.OnConnectedInterval, typeof(TimedIntervalGameEvent));
    Register(GameEvent.OnNewHighScore, typeof(NewHighScoreGameEvent));
    // This is only special because the skills system can only listen to enums for now
    // And cozmo shouldn't level up in solo mode.
    // TODO: can be removed when GameEvents have a baseclass instead of an enum or more filtering is on skillsystem.
    Register(GameEvent.OnMemoryMatchVsComplete, typeof(ChallengeGameEvent));
  }

  private static void Register(Anki.Cozmo.GameEvent Enum, Type type) {
    _EnumWrappers.Add(Enum, type);
  }
}
