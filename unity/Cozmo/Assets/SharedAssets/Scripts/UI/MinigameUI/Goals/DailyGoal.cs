using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo {
  namespace UI {
    [System.Serializable]
    public class DailyGoal {
      
      public GameEvent GoalEvent;
      public LocalizedString Title;
      public LocalizedString Description;
      public Sprite GoalIcon;
      public int Progress;
      public int Target;
      public int PointsRewarded;

      private bool _Completed;

      public bool GoalComplete {
        get {
          return (Progress >= Target);
        }
      }

      // TODO: Add some Action or Event (Probably into DailyGoalManager) that fires "OnDailyGoalComplete" Event and passes in itself when target is met
      // TODO: Replace PointsRewarded with a more generalized "Reward" class. Potentially replace
      // with an Action instead to match QuestEngine.
      // Example : ActionGrantReward (Reward) but we can also use it to trigger other things such as ActionGoalFeedback (VFX/Audio) or ActionAlertPopup (Text)
      // TODO: Create Trigger Conditions to allow for more situation based events.
      // Example : Replace SpeedTapSessionWin with MinigameSessionEnded, but the related Goal would then
      // have a MinigameTypeCondition (SpeedTap) and a DidWinCondition (True).

      // Action that fires when this Daily Goal is updated, passes through the DailyGoal itself so listeners can handle it.
      public Action<DailyGoal> OnDailyGoalUpdated;
      public Action<DailyGoal> OnDailyGoalCompleted;

      public DailyGoal(GameEvent gEvent, LocalizedString title, LocalizedString desc, Sprite icon, int reward, int target, int currProg = 0) {
        GoalEvent = gEvent;
        Title = title;
        Description = desc;
        GoalIcon = icon;
        PointsRewarded = reward;
        Target = target;
        Progress = currProg;
        _Completed = GoalComplete;
        GameEventManager.Instance.OnGameEvent += ProgressGoal;
      }

      private void ProgressGoal(GameEvent gEvent) {
        if (gEvent != GoalEvent) {
          return;
        }
        // TODO: Check Availability Conditions
        // Return false if false.
        // TODO: Check Trigger Conditions
        // Return false if false.
        // Progress Goal
        Progress++;
        DAS.Event(this, string.Format("{0} Progressed to {1}", Title, Progress));
        // Check if Completed
        if (GoalComplete && _Completed) {
          // Grant Reward
          // TODO: Use a more generic Reward Action
          DAS.Event(this, string.Format("{0} Completed", Title));
          PlayerManager.Instance.AddGreenPoints(PointsRewarded);
          if (OnDailyGoalCompleted != null) {
            OnDailyGoalCompleted.Invoke(this);
          }
        }
        if (OnDailyGoalUpdated != null) {
          OnDailyGoalUpdated.Invoke(this);
        }
      }
    }
  }
}