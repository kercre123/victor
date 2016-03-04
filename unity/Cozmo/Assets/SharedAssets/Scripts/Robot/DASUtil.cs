using System;

public static class DASUtil {
  public enum ViewType {
    View,
    Modal,
    Alert
  }

  public static string FormatDate(DataPersistence.Date date) {
    return date.ToString();
  }

  public static string FormatStatAmount(Anki.Cozmo.ProgressionStatType type, int amount) {
    return string.Format("{0}_{1}", type.ToString().ToLower(), amount);
  }

  public static string FormatGoal(Anki.Cozmo.ProgressionStatType type, int currentAmount, int amountNeeded) {
    return string.Format("{0}_{1}/{2}", type.ToString().ToLower(), currentAmount, amountNeeded);
  }

  public static string FormatViewTypeForOpen(ViewType type) {
    return string.Format("ui.{0}.enter", GetStringFromViewType(type));
  }

  public static string FormatViewTypeForClose(ViewType type) {
    return string.Format("ui.{0}.exit", GetStringFromViewType(type));
  }

  private static string GetStringFromViewType(ViewType type) {
    string viewString;
    switch (type) {
    case ViewType.Modal:
      viewString = "modal";
      break;
    case ViewType.Alert:
      viewString = "alert";
      break;
    case ViewType.View:
    default:
      viewString = "view";
      break;
    }
    return viewString;
  }
}