using System;
using System.Runtime.InteropServices;
using UnityEngine;

public static class DAS {
  public const int LogLevelDebug = 0;
  public const int LogLevelInfo = 1;
  public const int LogLevelEvent = 2;
  public const int LogLevelWarn = 3;
  public const int LogLevelError = 4;

  public static void Debug(string eventName, string eventValue) {
    PrintLn(LogLevelDebug, eventName, eventValue);
  }

  public static void Info(string eventName, string eventValue) {
    PrintLn(LogLevelInfo, eventName, eventValue);
  }

  public static void Event(string eventName, string eventValue) {
    PrintLn(LogLevelEvent, eventName, eventValue);
  }

  public static void Warn(string eventName, string eventValue) {
    #if UNITY_EDITOR
    UnityEngine.Debug.LogWarning(eventName + " " + eventValue);
    #else
    PrintLn(LogLevelWarn, eventName, eventValue);
    #endif
  }

  public static void Error(string eventName, string eventValue) {
    #if UNITY_EDITOR
    UnityEngine.Debug.LogError(eventName + " " + eventValue);
    #else
    PrintLn(LogLevelError, eventName, eventValue);
    #endif
  }

  public static void PrintLn(int dasLogLevel, string eventName, string eventValue) {
    #if DAS_ENABLED && !UNITY_EDITOR && (UNITY_ANDROID || UNITY_IPHONE || UNITY_STANDALONE)
    Unity_DAS_Log(dasLogLevel, eventName, eventValue);
    #else
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", dasLogLevel, eventName, eventValue));
    #endif
  }

  [DllImport("__Internal")]
  private static extern void Unity_DAS_Log(int level, string eventName, string eventValue);

}
