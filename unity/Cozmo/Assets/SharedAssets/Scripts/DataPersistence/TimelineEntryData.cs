using System;
using System.Collections.Generic;

namespace DataPersistence {
  public class TimelineEntryData {

    public TimelineEntryData(DateTime date) {
      Date = date;
    }

    public readonly DateTime Date;

    public readonly StatContainer Goals = new StatContainer();

    public readonly StatContainer Progress = new StatContainer();

    public float PlayTime;

    public readonly List<CompletedChallengeData> CompletedChallenges = new List<CompletedChallengeData>();
  }

  public class CompletedChallengeData {
    public DateTime StartTime;

    public DateTime EndTime;

    public readonly StatContainer AvailableStats = new StatContainer();

    public readonly StatContainer RecievedStats = new StatContainer();

    public string ChallengeId;
  }
}

