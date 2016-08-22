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
    public class DailyGoal : IDisposable {

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

      private bool _Completed = false;

      public bool GoalComplete {
        get {
          return (Progress >= Target);
        }
      }


      // Action that fires when this Daily Goal is updated, passes through the DailyGoal itself so listeners can handle it.
      [JsonIgnore]
      public Action<DailyGoal> OnDailyGoalUpdated;
      [JsonIgnore]
      public Action<DailyGoal> OnDailyGoalCompleted;

      // Conditions that must be met in order for this to progress when its event is fired.
      public List<GoalCondition> ProgConditions = new List<GoalCondition>();

      // Generate a daily goal from parameters
      [JsonConstructor]
      public DailyGoal(GameEvent gEvent, string titleKey, int reward, int target, string rewardType, List<GoalCondition> triggerCon, int priority = 0, int currProg = 0) {
        GoalEvent = gEvent;
        Title = new LocalizedString();
        Title.Key = titleKey;
        PointsRewarded = reward;
        Target = target;
        Progress = currProg;
        _Completed = GoalComplete;
        RewardType = rewardType;
        ProgConditions = triggerCon;
        Priority = priority;
        GameEventManager.Instance.OnGameEvent += ProgressGoal;
      }

      // Generate a fresh Daily Goal from data
      public DailyGoal(DailyGoalGenerationData.GoalEntry goalData) {
        GoalEvent = goalData.CladEvent;
        Title = new LocalizedString();
        Title.Key = goalData.TitleKey;
        PointsRewarded = goalData.PointsRewarded;
        Target = goalData.Target;
        Progress = 0;
        _Completed = GoalComplete;
        RewardType = goalData.RewardType;
        ProgConditions = goalData.ProgressConditions;
        Priority = goalData.Priority;
        GameEventManager.Instance.OnGameEvent += ProgressGoal;
      }

      public void Dispose() {
        GameEventManager.Instance.OnGameEvent -= ProgressGoal;
      }

      public void ProgressGoal(GameEventWrapper gEvent) {
        if (gEvent.GameEventEnum != GoalEvent) {
          return;
        }
        // If ProgConditions aren't met, don't progress
        if (!CanProgress(gEvent)) {
          return;
        }
        // Progress Goal
        Progress++;
        GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnDailyGoalProgress, this));

        DAS.Event(this, string.Format("{0} Progressed to {1}", Title, Progress));
        // Check if Completed
        CheckIfComplete();
        if (OnDailyGoalUpdated != null) {
          OnDailyGoalUpdated.Invoke(this);
        }
      }

      public void DebugAddGoalProgress() {
        // Progress Goal
        Progress++;
        GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnDailyGoalProgress, this));

        DAS.Event(this, string.Format("{0} Progressed to {1}", Title, Progress));
        // Check if Completed
        CheckIfComplete();
        if (OnDailyGoalUpdated != null) {
          OnDailyGoalUpdated.Invoke(this);
        }

      }

      public void DebugSetGoalProgress(int prog) {
        Progress = prog;
        if (!GoalComplete && _Completed) {
          _Completed = false;
          GameEventManager.Instance.OnGameEvent += ProgressGoal;
        }
        else if (_Completed == false) {
          CheckIfComplete();
        }
        if (OnDailyGoalUpdated != null) {
          OnDailyGoalUpdated.Invoke(this);
        }

      }

      public void DebugUndoGoalProgress() {
        if (Progress > 0) {
          Progress--;
          if (!GoalComplete && _Completed) {
            _Completed = false;
            GameEventManager.Instance.OnGameEvent += ProgressGoal;
          }
          if (OnDailyGoalUpdated != null) {
            OnDailyGoalUpdated.Invoke(this);
          }
        }

      }

      public void DebugResetGoalProgress() {
        Progress = 0;
        if (_Completed) {
          _Completed = false;
          GameEventManager.Instance.OnGameEvent += ProgressGoal;
        }
        if (OnDailyGoalUpdated != null) {
          OnDailyGoalUpdated.Invoke(this);
        }

      }

      /// <summary>
      /// Checks if the goal has just been completed, handles any logic that is fired when
      /// a goal is completed.
      /// </summary>
      public void CheckIfComplete() {
        if (GoalComplete && _Completed == false) {
          // Grant Reward
          DAS.Event(this, string.Format("{0} Completed", Title));
          DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount(RewardType, PointsRewarded);
          if (OnDailyGoalCompleted != null) {
            OnDailyGoalCompleted.Invoke(this);
          }
          GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnDailyGoalCompleted, this));
          _Completed = true;
          GameEventManager.Instance.OnGameEvent -= ProgressGoal;
        }
      }

      public bool CanProgress(GameEventWrapper gEvent) {
        if (ProgConditions == null) {
          return true;
        }
        for (int i = 0; i < ProgConditions.Count; i++) {
          if (ProgConditions[i].ConditionMet(gEvent) == false) {
            return false;
          }
        }
        return true;
      }
    }
  }
}