using System;
using System.Runtime.InteropServices;
using UnityEngine;

public class IphoneDasTarget : IDASTarget {

  public void Event(string eventName, string eventValue, object context = null) {
  
 #if !UNITY_EDITOR && UNITY_IPHONE
    Unity_DAS_Event(eventName, eventValue);
 #endif
  }
 
  public void Error(string eventName, string eventValue, object context = null) {

#if !UNITY_EDITOR && UNITY_IPHONE
    Unity_DAS_LogE(eventName, eventValue);
#endif
  }

  public void Warn(string eventName, string eventValue, object context = null) {

#if !UNITY_EDITOR && UNITY_IPHONE
      Unity_DAS_LogW(eventName, eventValue);
#endif
    }

  public void Info(string eventName, string eventValue, object context = null) {

#if !UNITY_EDITOR && UNITY_IPHONE
    Unity_DAS_LogI(eventName, eventValue);
#endif
  }

  public void Debug(string eventName, string eventValue, object context = null) {

#if !UNITY_EDITOR && UNITY_IPHONE
    Unity_DAS_LogD(eventName, eventValue);
#endif
  }

#if UNITY_IPHONE
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
  #endif
}
