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

      // Action that fires when this Daily Goal is updated, passes through the DailyGoal itself so listeners can handle it.
      [JsonIgnore]
      public Action<DailyGoal> OnDailyGoalUpdated;
      [JsonIgnore]
      public Action<DailyGoal> OnDailyGoalCompleted;

      // Conditions that must be met in order for this to progress when its event is fired.
      public List<GoalCondition> ProgConditions = new List<GoalCondition>();

      private List<GoalCondition> GenConditions = new List<GoalCondition>();

      // Generate a daily goal from parameters
      [JsonConstructor]
      public DailyGoal(GameEvent gEvent, string titleKey, int reward, int target, string rewardType, List<GoalCondition> triggerCon, List<GoalCondition> genConds, int priority = 0, int currProg = 0) {
        GoalEvent = gEvent;
        Title = new LocalizedString();
        Title.Key = titleKey;
        PointsRewarded = reward;
        Target = target;
        Progress = currProg;
        _GoalComplete = IsGoalComplete();
        RewardType = rewardType;
        ProgConditions = triggerCon;
        GenConditions = genConds;
        Priority = priority;
      }

      // Generate a fresh Daily Goal from data
      public DailyGoal(DailyGoalGenerationData.GoalEntry goalData) {
        GoalEvent = goalData.CladEvent;
        Title = new LocalizedString();
        Title.Key = goalData.TitleKey;
        PointsRewarded = goalData.PointsRewarded;
        Target = goalData.Target;
        Progress = 0;
        _GoalComplete = false;
        RewardType = goalData.RewardType;
        ProgConditions = goalData.ProgressConditions;
        Priority = goalData.Priority;
        GenConditions = goalData.GenConditions;
      }


      public void DebugAddGoalProgress() {
        // Progress Goal
        Progress++;
        GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnDailyGoalProgress, this));

        DAS.Event("meta.daily_goal.progress", Title.Key, DASUtil.FormatExtraData(Progress.ToString()));
        // Check if Completed
        CheckIfComplete();
        if (OnDailyGoalUpdated != null) {
          OnDailyGoalUpdated.Invoke(this);
        }

      }

      public void DebugSetGoalProgress(int prog, bool fireRewards = true) {
        Progress = prog;
        _GoalComplete = !fireRewards;
        CheckIfComplete();
        if (OnDailyGoalUpdated != null) {
          OnDailyGoalUpdated.Invoke(this);
        }

      }

      public void DebugUndoGoalProgress() {
        if (Progress > 0) {
          Progress--;
          _GoalComplete = IsGoalComplete();
          if (OnDailyGoalUpdated != null) {
            OnDailyGoalUpdated.Invoke(this);
          }
        }

      }

      public void DebugResetGoalProgress() {
        Progress = 0;
        _GoalComplete = false;
        if (OnDailyGoalUpdated != null) {
          OnDailyGoalUpdated.Invoke(this);
        }

      }

      /// <summary>
      /// Checks if the goal has just been completed, handles any logic that is fired when
      /// a goal is completed.
      /// </summary>
      public void CheckIfComplete() {
        if (IsGoalComplete() && !GoalComplete) {
          // Grant Reward
          int numDays = 0;
          if (DataPersistenceManager.Instance.Data.DefaultProfile.Sessions != null) {
            numDays = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Count;
          }
          DAS.Event("meta.daily_goal_completed", numDays.ToString(), DASUtil.FormatExtraData(Title.Key));
          DAS.Event("meta.daily_goal_reward", PointsRewarded.ToString());
          _GoalComplete = true;
          if (OnDailyGoalCompleted != null) {
            OnDailyGoalCompleted.Invoke(this);
          }
          GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnDailyGoalCompleted, this));
        }
      }

      public void CheckAndResolveInvalidGenConditions() {
        // Force a daily goal complete if its gen conditions are not met but it is still
        // in your daily goal list. This should only turn up if something goes weird with unlocks.
        // Don't force complete if we haven't progressed to that point.
        // Connecting to a mature account with less mature cozmo
        // will require you to generate fresh goals
        if (!GoalComplete && GenConditions != null && GenConditions.Count > 0) {
          for (int i = 0; i < GenConditions.Count; i++) {
            // If the GenCondition is not met, we may be in a state where this goal should have
            // already been earned
            if (!GenConditions[i].ConditionMet()) {
              // Check for Unlockables progress
              if (GenConditions[i] is CurrentUnlockCondition) {
                if (ValidateUnlockIdGenCondition(GenConditions[i] as CurrentUnlockCondition)) {
                  DebugSetGoalProgress(Target, false);
                  return;
                }
              }// Check for Difficulty Level Progress
              else if (GenConditions[i] is CurrentDifficultyUnlockedCondition) {
                if (ValidateDifficultyGenCondtion(GenConditions[i] as CurrentDifficultyUnlockedCondition)) {
                  DebugSetGoalProgress(Target, false);
                  return;
                }
              }

            }
          }
        }
      }

      // Check if the CurrentUnlockCondition should have already been completed
      public bool ValidateUnlockIdGenCondition(CurrentUnlockCondition unlockCond) {
        bool forceComplete = false;
        if (UnlockablesManager.Instance.IsUnlocked(unlockCond.Unlocked)) {
          // We already unlocked this but don't want this goal unlocked for generation, force complete
          if (unlockCond.State == UnlockableState.Available || unlockCond.State == UnlockableState.Locked) {
            forceComplete = true;
          }
        }
        return forceComplete;
      }

      // Check if the CurrentDifficultyUnlockedCondition should have already been completed
      public bool ValidateDifficultyGenCondtion(CurrentDifficultyUnlockedCondition diffCond) {
        bool forceComplete = false;
        // Only force complete if we have a higher difficulty unlocked than expected
        int diff = 0;
        if (DataPersistenceManager.Instance.Data.DefaultProfile.GameDifficulty.TryGetValue(diffCond.ChallengeID, out diff)) {
          if (diffCond.UseMaxDiff && diff > diffCond.MaxDiff) {
            forceComplete = true;
          }
        }

        return forceComplete;
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

      private bool IsGoalComplete() {
        return Progress >= Target;
      }
    }
  }
}