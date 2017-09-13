using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public static class IOS_Settings {
  [DllImport("__Internal")]
  private static extern void exportCodelabFile(string projectNameString, string projectContentString);

  [DllImport("__Internal")]
  private static extern void openAppSettings();

  public static void OpenAppSettings() {
#if UNITY_IOS && !UNITY_EDITOR
    openAppSettings();
#endif
  }

  public static void ExportCodelabFile(string projectNameString, string projectContentString) {
#if UNITY_IOS && !UNITY_EDITOR
    exportCodelabFile(projectNameString, projectContentString);
#endif
  }
}
