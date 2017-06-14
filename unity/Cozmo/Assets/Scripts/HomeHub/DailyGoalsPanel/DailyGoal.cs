using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using DataPersistence;
using Newtonsoft.Json;

namespace Cozmo {
  namespace UI {
    [System.Serializable]
    public class DailyGoal {

      public GameEvent GoalEvent;
      public LocalizedString Title;
      public string RewardType;
      private int _Progress;

      public int Progress {
        get {
          return _Progress;
        }
        set {
          _Progress = Mathf.Min(value, Target);
        }
      }

      public int Target;
      public int PointsRewarded;
      public int Priority = 0;

      private bool _GoalComplete;
      public bool GoalComplete {
        get {
          return _GoalComplete;
        }
        set {
          _GoalComplete = value;
        }
      }

      // Conditions that must be met in order for this to progress when its event is fired.
      public List<GoalCondition> ProgConditions = new List<GoalCondition>();

      // Generate a daily goal from parameters
      [JsonConstructor]
      public DailyGoal(GameEvent gEvent, string titleKey, int reward, int target, string rewardType, List<GoalCondition> triggerCon, List<GoalCondition> genConds, int priority = 0, int currProg = 0) {
        GoalEvent = gEvent;
        Title = new LocalizedString();
        Title.Key = titleKey;
        PointsRewarded = reward;
        Target = target;
        Progress = currProg;
        _GoalComplete = (Progress >= Target);
        RewardType = rewardType;
        ProgConditions = triggerCon;
        Priority = priority;
      }
    }
  }
}