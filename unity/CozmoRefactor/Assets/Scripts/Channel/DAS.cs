using System;
using System.Runtime.InteropServices;
using UnityEngine;

#if (!UNITY_EDITOR && (UNITY_ANDROID || UNITY_IPHONE || UNITY_STANDALONE)) || ANIMATION_TOOL
public static class DAS {
   
  public static void Event(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    Unity_DAS_Event(eventName, eventValue);
#endif
  }
 
  public static void Error(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    Unity_DAS_LogE(eventName, eventValue);
#endif
  }

  public static void Warn(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
      Console.Out.WriteLine(eventName + ": " + eventValue);

#else
      Unity_DAS_LogW(eventName, eventValue);
#endif
    }

    public static void Info(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    Unity_DAS_LogI(eventName, eventValue);
#endif
  }

  public static void Debug(string eventName, string eventValue, UnityEngine.Object context = null) {

#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    Unity_DAS_LogD(eventName, eventValue);
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