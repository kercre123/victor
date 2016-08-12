using System;
using Cozmo.UI;
using System.Collections.Generic;

namespace DataPersistence {
  public class TimelineEntryData {
    public Date Date;

    public List<DailyGoal> DailyGoals;

    public float GetTotalProgress() {
      float totalG = 0.0f;
      float totalP = 0.0f;
      for (int i = 0; i < DailyGoals.Count; i++) {
        totalG += DailyGoals[i].Target;
        totalP += DailyGoals[i].Progress;
      }
      float finalP = totalP / totalG;
      return finalP;
    }

    public float PlayTime;

    public readonly List<CompletedChallengeData> CompletedChallenges;

    // The entry is considered complete after the friendship points have been awarded.
    public bool Complete;

    // Is true once all daily goals have been completed
    public bool GoalsFinished;

    public int ChestsGained;

    public Dictionary<Anki.Cozmo.UnlockId, int> SparkCount;

    public TimelineEntryData() {
      CompletedChallenges = new List<CompletedChallengeData>();
      DailyGoals = new List<DailyGoal>();
      SparkCount = new Dictionary<Anki.Cozmo.UnlockId, int>();
    }

    public TimelineEntryData(Date date) : this() {
      Date = date;
    }
  }

  public class CompletedChallengeData {
    public DateTime StartTime;

    public DateTime EndTime;

    public string ChallengeId;

    public bool Won;

    public CompletedChallengeData() {
      // TODO: Include Count of how often Rewarded events triggered?
    }
  }
}

