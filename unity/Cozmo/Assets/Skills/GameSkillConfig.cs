using UnityEngine;
using System.Collections;

using System.Collections.Generic;

// Stored as part of the game config.
// Contains designer entered data about the skill levels.

[System.Serializable]
public class GameSkillTuple {
  public string name;
  public float val;
}

[System.Serializable]
public class GameSkillLevelConfig {
  // Editor facing version.
  public List<GameSkillTuple> _skills;
  // Unity doesn't support dictionary in editors so just making a copy when we use it.
  private Dictionary<string,float> _skillVals;
  public float _minThreshold;
  public float _maxThreshold;
}

[System.Serializable]
public class GameSkillConfig {
  public Anki.Cozmo.GameEvent _triggerEvent;

  public GameSkillLevelConfig[] _levels;
}
