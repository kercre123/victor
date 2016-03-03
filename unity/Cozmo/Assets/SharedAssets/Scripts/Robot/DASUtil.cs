using System;

public static class DASUtil {
  public static string FormatDate(DataPersistence.Date date) {
    return date.ToString();
  }

  public static string FormatStatAmount(Anki.Cozmo.ProgressionStatType type, int amount) {
    return string.Format("{0}_{1}", type.ToString().ToLower(), amount);
  }

  public static string FormatGoal(Anki.Cozmo.ProgressionStatType type, int currentAmount, int amountNeeded) {
    return string.Format("{0}_{1}/{2}", type.ToString().ToLower(), currentAmount, amountNeeded);
  }
}