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

public class MinigameStartedGameEvent : GameEventWrapper {
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

public class MinigameGameEvent : GameEventWrapper {
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

public class UnlockableUnlockedGameEvent : GameEventWrapper {
  public UnlockId Unlock;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);
    if (args.Length > 0 && args[0].GetType() == typeof(UnlockId)) {
      Unlock = (UnlockId)args[0];
    }
  }

}

public class DailyGoalCompleteGameEvent : GameEventWrapper {
  public GameEvent GoalEvent;
  public int RewardCount;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);

    if (args.Length > 0 && args[0].GetType() == typeof(GameEvent)) {
      GoalEvent = (GameEvent)args[0];
    }

    if (args.Length > 1 && args[1].GetType() == typeof(int)) {
      RewardCount = (int)args[1];
    }
  }
}

public class DailyGoalProgressGameEvent : GameEventWrapper {
  public GameEvent GoalEvent;
  public int Progress;
  public int Target;

  public override void Init(GameEvent Enum, params object[] args) {
    base.Init(Enum);

    if (args.Length > 0 && args[0].GetType() == typeof(GameEvent)) {
      GoalEvent = (GameEvent)args[0];
    }

    if (args.Length > 1 && args[1].GetType() == typeof(int)) {
      Progress = (int)args[1];
    }

    if (args.Length > 2 && args[2].GetType() == typeof(int)) {
      Target = (int)args[2];
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
    Register(GameEvent.OnChallengeStarted, typeof(MinigameStartedGameEvent));
    Register(GameEvent.OnChallengePointScored, typeof(MinigameGameEvent));
    Register(GameEvent.OnChallengeRoundEnd, typeof(MinigameGameEvent));
    Register(GameEvent.OnChallengeComplete, typeof(MinigameGameEvent));
    Register(GameEvent.OnChallengeDifficultyUnlock, typeof(DifficultyUnlockedGameEvent));
    Register(GameEvent.OnUnlockableEarned, typeof(UnlockableUnlockedGameEvent));
    Register(GameEvent.OnDailyGoalProgress, typeof(DailyGoalProgressGameEvent));
    Register(GameEvent.OnDailyGoalCompleted, typeof(DailyGoalCompleteGameEvent));
    Register(GameEvent.OnNewDayStarted, typeof(SessionStartedGameEvent));
  }

  private static void Register(Anki.Cozmo.GameEvent Enum, Type type) {
    _EnumWrappers.Add(Enum, type);
  }
}
