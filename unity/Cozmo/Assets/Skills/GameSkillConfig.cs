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
  public GameSkillTuple[] SkillsArray;

  private Dictionary<string,float> _SkillMap = new Dictionary<string,float>();

  public void OnBeforeSerialize() {
  }

  public void OnAfterDeserialize() {
    _SkillMap = new Dictionary<string,float>();
    for (int i = 0; i < SkillsArray.Length; ++i) {
      if (_SkillMap.ContainsKey(SkillsArray[i].name)) {
        _SkillMap[SkillsArray[i].name] = SkillsArray[i].val;
      }
      else {
        _SkillMap.Add(SkillsArray[i].name, SkillsArray[i].val);
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

  [SerializeField]
  private bool _UsePointThreshold;
  // for when you only compare after X points scored.
  [SerializeField]
  private int _ComparedPointThreshold;
  [SerializeField]
  private GameSkillLevelConfig[] _Levels;
}
