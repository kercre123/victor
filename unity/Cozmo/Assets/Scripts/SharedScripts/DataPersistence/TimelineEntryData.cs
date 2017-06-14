using System;
using Cozmo.UI;
using System.Collections.Generic;

namespace DataPersistence {
  public class TimelineEntryData {
    public Date Date;

    public float PlayTime;

    public bool HasConnectedToCozmo;

    public readonly List<CompletedChallengeData> CompletedChallenges;

    public Dictionary<string, int> TotalWins;

    public Dictionary<Anki.Cozmo.UnlockId, int> SparkCount;

    public TimelineEntryData() {
      CompletedChallenges = new List<CompletedChallengeData>();
      SparkCount = new Dictionary<Anki.Cozmo.UnlockId, int>();
      TotalWins = new Dictionary<string, int>();
      PlayTime = 0.0f;
      HasConnectedToCozmo = false;
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

