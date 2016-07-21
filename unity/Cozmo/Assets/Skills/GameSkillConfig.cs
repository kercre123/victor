using UnityEngine;
using System.Collections;
using System.Linq;

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
  // Level down if winning more than 70% of the time, Level up if we're winning less than 30%
  [Range(0.0f, 1.0f), Tooltip("Cozmo becomes worse if winning more than UpperBound %")]
  public float UpperBoundThreshold = 0.7f;
  [Range(0.0f, 1.0f), Tooltip("Cozmo becomes better if winning less than LowerBound %")]
  public float LowerBoundThreshold = 0.3f;

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

  [SerializeField, Tooltip("Event that evaluates win rate to determine if skill should raise/lower")]
  private List<SerializableGameEvents> _EvaluateLevelChangeEvent;
  [SerializeField, Tooltip("Event that records points based on if Player or Cozmo won")]
  private List<SerializableGameEvents> _ChallengePointEvent;
  [SerializeField, Tooltip("Event that clears recorded points, usually when you start game")]
  private List<SerializableGameEvents> _ResetPointsEvent;

  public bool IsLevelChangeEvent(Anki.Cozmo.GameEvent gameEvent) {
    return _EvaluateLevelChangeEvent != null && _EvaluateLevelChangeEvent.Find(x => x.Value == gameEvent) != null;
  }

  public bool IsChallengePointEvent(Anki.Cozmo.GameEvent gameEvent) {
    return _ChallengePointEvent != null && _ChallengePointEvent.Find(x => x.Value == gameEvent) != null;
  }

  public bool IsResetPointEvent(Anki.Cozmo.GameEvent gameEvent) {
    return _ResetPointsEvent != null && _ResetPointsEvent.Find(x => x.Value == gameEvent) != null;
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

  [SerializeField, Tooltip("Only evaluate after a certain number of points scored if desired so players can't just quit.")]
  private bool _UsePointThreshold;
  // for when you only compare after X points scored.
  [SerializeField, Tooltip("Points scored total needed in order to attempt an evaluation")]
  private int _ComparedPointThreshold;


  [SerializeField]
  private GameSkillLevelConfig[] _Levels;
}
// end GameSkillConfig
