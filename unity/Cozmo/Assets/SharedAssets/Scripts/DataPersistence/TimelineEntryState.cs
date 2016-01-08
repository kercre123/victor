using System;

namespace DataPersistence {
  public class TimelineEntryState {

    public TimelineEntryState(DateTime date) {
      Date = date;
    }

    public readonly DateTime Date;

    public readonly StatContainer Goals = new StatContainer();

    public readonly StatContainer Progress = new StatContainer();

    public float PlayTime;
  }
}

