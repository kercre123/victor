using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public static class IOS_Settings {
  [DllImport("__Internal")]
  private static extern bool exportCodelabFile(string projectNameString, string projectContentString);

  [DllImport("__Internal")]
  private static extern void openAppSettings();

  public static void OpenAppSettings() {
#if UNITY_IOS && !UNITY_EDITOR
    openAppSettings();
#endif
  }

  public static bool ExportCodelabFile(string projectNameString, string projectContentString) {
#if UNITY_IOS && !UNITY_EDITOR
    return exportCodelabFile(projectNameString, projectContentString);
#else
    return false;
#endif
  }
}
