using System;
using System.Collections.Generic;

public static class DASUtil {
  public static string FormatDate(DataPersistence.Date date) {
    return date.ToString();
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

  //
  // Maximum length allowed for stack traces reported to DAS.
  //
  public const int MAX_STACK_LEN = 512;

  //
  // Sanitize unity stack trace for reporting to DAS.
  // Returns string of length <= MAX_STACK_LEN.
  // Newlines are replaced by space for readability.
  //
  public static string FormatStackTrace(string stackTrace) {
    if (stackTrace != null) {
      if (stackTrace.Length > MAX_STACK_LEN) {
        stackTrace = stackTrace.Remove(MAX_STACK_LEN);
      }
      stackTrace = stackTrace.Replace('\n', ' ');
    }
    return stackTrace;
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