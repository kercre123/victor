using System;
using System.Collections.Generic;

public static class DASUtil {
  public static string FormatDate(DataPersistence.Date date) {
    return date.ToString();
  }

  public static string FormatStatAmount(Anki.Cozmo.ProgressionStatType type, int amount) {
    return string.Format("{0}_{1}", type.ToString().ToLower(), amount);
  }

  public static string FormatGoal(Cozmo.UI.DailyGoal dailyGoal) {
    return string.Format("{0}_{1}/{2}", dailyGoal.Title, dailyGoal.Progress, dailyGoal.Target);
  }

  public static string FormatViewTypeForOpen(Cozmo.UI.BaseDialog dialog) {
    return string.Format("ui.{0}.enter", GetStringFromViewType(dialog));
  }

  public static string FormatViewTypeForClose(Cozmo.UI.BaseDialog dialog) {
    return string.Format("ui.{0}.exit", GetStringFromViewType(dialog));
  }

  public static Dictionary<string, string> FormatExtraData(string str) {
    Dictionary<string, string> DASData = new Dictionary<string, string>();
    DASData.Add("$data", str);
    return DASData;
  }

  private static string GetStringFromViewType(Cozmo.UI.BaseDialog dialog) {
    string viewString = "dialog";
    if (dialog is Cozmo.UI.AlertModal) {
      viewString = "alert";
    }
    else if (dialog is Cozmo.UI.BaseModal) {
      viewString = "modal";
    }
    else if (dialog is Cozmo.UI.BaseView) {
      viewString = "view";
    }
    return viewString;
  }
}