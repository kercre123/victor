using System;
using System.Runtime.InteropServices;
using UnityEngine;

#if (!UNITY_EDITOR && (UNITY_ANDROID || UNITY_IPHONE || UNITY_STANDALONE)) || ANIMATION_TOOL
public static class DAS {

  public delegate void DASHandler(string eventName, string eventValue, UnityEngine.Object context);
  public static event DASHandler OnEventLogged;
  public static event DASHandler OnErrorLogged;
  public static event DASHandler OnWarningLogged;
  public static event DASHandler OnInfoLogged;
  public static event DASHandler OnDebugLogged;

  public static void Event(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    Unity_DAS_Event(eventName, eventValue);
    if (OnEventLogged != null){
      OnEventLogged(eventName, eventValue, context);
    }
#endif
  }
 
  public static void Error(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    Unity_DAS_LogE(eventName, eventValue);
    if (OnErrorLogged != null){
      OnErrorLogged(eventName, eventValue, context);
    }
#endif
  }

  public static void Warn(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
      Console.Out.WriteLine(eventName + ": " + eventValue);

#else
      Unity_DAS_LogW(eventName, eventValue);
    if (OnWarningLogged != null){
      OnWarningLogged(eventName, eventValue, context);
    }
#endif
    }

    public static void Info(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    Unity_DAS_LogI(eventName, eventValue);
    if (OnInfoLogged != null){
      OnInfoLogged(eventName, eventValue, context);
    }
#endif
  }

  public static void Debug(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    Unity_DAS_LogD(eventName, eventValue);
    if (OnDebugLogged != null){
      OnDebugLogged(eventName, eventValue, context);
    }
#endif
  }

  // TODO: __Internal only works on certain platforms (iOS, but not Android, etc)
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
#endif