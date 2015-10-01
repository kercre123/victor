using System;
using System.Runtime.InteropServices;
using UnityEngine;

public static partial class DAS {

  public static void Event(string eventName, string eventValue, UnityEngine.Object context = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 3, eventName, eventValue), context);
  }

  public static void Error(string eventName, string eventValue, UnityEngine.Object context = null) {
    UnityEngine.Debug.LogError(string.Format("DAS [{0}] {1} - {2}", 5, eventName, eventValue), context);
  }

  public static void Warn(string eventName, string eventValue, UnityEngine.Object context = null) {
    UnityEngine.Debug.LogWarning(string.Format("DAS [{0}] {1} - {2}", 4, eventName, eventValue), context);
  }

  public static void Info(string eventName, string eventValue, UnityEngine.Object context = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 2, eventName, eventValue), context);
  }

  public static void Debug(string eventName, string eventValue, UnityEngine.Object context = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 1, eventName, eventValue), context);
  }
  
}
