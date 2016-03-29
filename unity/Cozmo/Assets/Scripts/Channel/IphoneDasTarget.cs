using System;
using System.Runtime.InteropServices;
using UnityEngine;
using System.Collections.Generic;

public class IphoneDasTarget : IDASTarget {

  public void Event(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {
    #if !UNITY_EDITOR && UNITY_IPHONE
    if (keyValues != null) {
      Unity_DAS_Event(eventName, eventValue, keyValues.Keys, keyValues.Values, keyValues.Count);
    } else {
      Unity_DAS_Event(eventName, eventValue, {}, {}, 0);
    }
    #endif
  }

  public void Error(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {

#if !UNITY_EDITOR && UNITY_IPHONE
    if (keyValues != null) {
      Unity_DAS_LogE(eventName, eventValue, keyValues.Keys, keyValues.Values, keyValues.Count);
    } else {
      Unity_DAS_LogE(eventName, eventValue, {}, {}, 0);
    }
#endif
  }

  public void Warn(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {

#if !UNITY_EDITOR && UNITY_IPHONE
    if (keyValues != null) {
      Unity_DAS_LogW(eventName, eventValue, keyValues.Keys, keyValues.Values, keyValues.Count);
    } else {
      Unity_DAS_LogW(eventName, eventValue, {}, {}, 0);
    }
#endif
  }

  public void Info(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {

#if !UNITY_EDITOR && UNITY_IPHONE
    if (keyValues != null) {
      Unity_DAS_LogI(eventName, eventValue, keyValues.Keys, keyValues.Values, keyValues.Count);
    } else {
      Unity_DAS_LogI(eventName, eventValue, {}, {}, 0);
    }
#endif
  }

  public void Debug(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {

#if !UNITY_EDITOR && UNITY_IPHONE
    if (keyValues != null) {
      Unity_DAS_LogD(eventName, eventValue, keyValues.Keys, keyValues.Values, keyValues.Count);
    } else {
      Unity_DAS_LogD(eventName, eventValue, {}, {}, 0);
    }
#endif
  }

  #if UNITY_IPHONE
  // TODO: __Internal only works on certain platforms (iOS, but not Android, etc)
  [DllImport("__Internal")]
  private static extern void Unity_DAS_Event(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);

  [DllImport("__Internal")]
  private static extern void Unity_DAS_LogE(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);

  [DllImport("__Internal")]
  private static extern void Unity_DAS_LogW(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);

  [DllImport("__Internal")]
  private static extern void Unity_DAS_LogI(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);

  [DllImport("__Internal")]
  private static extern void Unity_DAS_LogD(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);
  #endif
}
