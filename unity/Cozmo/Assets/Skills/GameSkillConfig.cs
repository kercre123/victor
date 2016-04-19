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
public class GameSkillLevelConfig: ISerializationCallbackReceiver {
  // Work Around for unity not having Serializable or Drawer of dictionaryies
  // And there are only a few skills per game so quering won't be that expensive to loop through
  [SerializeField]
  private GameSkillTuple[] SkillsArray;

  public Dictionary<string,float> SkillMap = new Dictionary<string,float>();
  // Level up if winning more than 70% of the time, Level down if we're losing more than 40%
  [Range(0.0f, 1.0f)]
  public float LevelUpMinThreshold = 0.7f;
  [Range(0.0f, 1.0f)]
  public float LevelDownMaxThreshold = 0.39f;

  public void OnBeforeSerialize() {
  }

  public void OnAfterDeserialize() {
    SkillMap = new Dictionary<string,float>();
    for (int i = 0; i < SkillsArray.Length; ++i) {
      if (SkillMap.ContainsKey(SkillsArray[i].name)) {
        SkillMap[SkillsArray[i].name] = SkillsArray[i].val;
      }
      else {
        SkillMap.Add(SkillsArray[i].name, SkillsArray[i].val);
      }
    }
  }
}

[System.Serializable]
public class GameSkillConfig {
  // Can't save gameEvents since they save as enums... Write custom serielizer and display...
  // Display for the dictionaries, String for serielizer

  [SerializeField]
  private SerializableGameEvents _EvaluateLevelChangeEvent;
  [SerializeField]
  private SerializableGameEvents _GainChallengePointEvent;
  [SerializeField]
  private SerializableGameEvents _LoseChallengePointEvent;

  public Anki.Cozmo.GameEvent EvaluateLevelChangeEvent {
    get { return _EvaluateLevelChangeEvent.Value; }
  }

  public Anki.Cozmo.GameEvent GainChallengePointEvent {
    get { return _GainChallengePointEvent.Value; }
  }

  public Anki.Cozmo.GameEvent LoseChallengePointEvent {
    get { return _LoseChallengePointEvent.Value; }
  }

  public bool UsePointThreshold {
    get { return _UsePointThreshold; }
  }

  public int ComparedPointThreshold {
    get { return _ComparedPointThreshold; }
  }

  public GameSkillLevelConfig GetCurrLevelConfig(int level) {
    if (_Levels != null) {
      if (level < 0) {
        return _Levels[0];
      }
      if (level >= GetMaxLevel()) {
        return _Levels[_Levels.Length - 1];
      }
      else {
        return _Levels[level];
      }
    }
    return null;
  }

  public int GetMaxLevel() {
    return _Levels != null ? _Levels.Length : 0;
  }

  [SerializeField]
  private bool _UsePointThreshold;
  // for when you only compare after X points scored.
  [SerializeField]
  private int _ComparedPointThreshold;


  [SerializeField]
  private GameSkillLevelConfig[] _Levels;
}
// end GameSkillConfig
