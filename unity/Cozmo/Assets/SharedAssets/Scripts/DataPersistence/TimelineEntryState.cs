using System;

namespace DataPersistence {
  public class TimelineEntryState {
    public TimelineEntryState() {
    }

    public DateTime Date;

    public StatContainer Goals;

    public StatContainer Progress;

    public float PlayTime;
  }
}

