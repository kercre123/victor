using System;
using System.Runtime.InteropServices;
using UnityEngine;

public static partial class DAS {

  public static void Event(string eventName, string eventValue, UnityEngine.Object context = null) {
    if (Application.isEditor) {
      UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 3, eventName, eventValue), context);
    }
    else {
      Unity_DAS_Event(eventName, eventValue);
    }
  }

  public static void Error(string eventName, string eventValue, UnityEngine.Object context = null) {
    if (Application.isEditor) {
      UnityEngine.Debug.LogError(string.Format("DAS [{0}] {1} - {2}", 5, eventName, eventValue), context);
    }
    else {
      Unity_DAS_LogE(eventName, eventValue);
    }
  }

  public static void Warn(string eventName, string eventValue, UnityEngine.Object context = null) {
    if (Application.isEditor) {
      UnityEngine.Debug.LogWarning(string.Format("DAS [{0}] {1} - {2}", 4, eventName, eventValue), context);
    }
    else {
      Unity_DAS_LogW(eventName, eventValue);
    }
  }

  public static void Info(string eventName, string eventValue, UnityEngine.Object context = null) {
    if (Application.isEditor) {
      UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 2, eventName, eventValue), context);
    }
    else {
      Unity_DAS_LogI(eventName, eventValue);
    }
  }

  public static void Debug(string eventName, string eventValue, UnityEngine.Object context = null) {
    if (Application.isEditor) {
      UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 1, eventName, eventValue), context);
    }
    else {
      Unity_DAS_LogD(eventName, eventValue);
    }
  }

  [DllImport("__Internal")]
  private static extern void Unity_DAS_Event(string eventName, string eventValue);

  [DllImport("__Internal")]
  private static extern void Unity_DAS_LogE(string eventName, string eventValue);

  [DllImport("__Internal")]
  private static extern void Unity_DAS_LogW(string eventName, string eventValue);

  [DllImport("__Internal")]
  private static extern void Unity_DAS_LogI(string eventName, string eventValue);

  [DllImport("__Internal")]
  private static extern void Unity_DAS_LogD(string eventName, string eventValue);

}
