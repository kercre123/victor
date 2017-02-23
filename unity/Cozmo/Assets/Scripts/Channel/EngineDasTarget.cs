#if !UNITY_EDITOR && (UNITY_IOS || UNITY_ANDROID)
#define USE_ENGINE_TARGET
#endif

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using System.Collections.Generic;
using System.Linq;

public class EngineDasTarget : IDASTarget {

  public void Event(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
#if USE_ENGINE_TARGET
    if (keyValues != null) {
      Unity_DAS_Event(eventName, eventValue, keyValues.Keys.ToArray(), keyValues.Values.ToArray(), (uint)keyValues.Count);
    } else {
      Unity_DAS_Event(eventName, eventValue, null, null, 0);
    }
#endif
  }

  public void Error(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {

#if USE_ENGINE_TARGET
    eventName = "unity."+eventName;
    if (keyValues != null) {
      Unity_DAS_LogE(eventName, eventValue, keyValues.Keys.ToArray(), keyValues.Values.ToArray(), (uint)keyValues.Count);
    } else {
      Unity_DAS_LogE(eventName, eventValue, null, null, 0);
    }
#endif
  }

  public void Warn(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {

#if USE_ENGINE_TARGET
    eventName = "unity."+eventName;
    if (keyValues != null) {
      Unity_DAS_LogW(eventName, eventValue, keyValues.Keys.ToArray(), keyValues.Values.ToArray(), (uint)keyValues.Count);
    } else {
      Unity_DAS_LogW(eventName, eventValue, null, null, 0);
    }
#endif
  }

  public void Info(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {

#if USE_ENGINE_TARGET
    eventName = "unity."+eventName;
    if (keyValues != null) {
      Unity_DAS_LogI(eventName, eventValue, keyValues.Keys.ToArray(), keyValues.Values.ToArray(), (uint)keyValues.Count);
    } else {
      Unity_DAS_LogI(eventName, eventValue, null, null, 0);
    }
#endif
  }

  public void Debug(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    
#if USE_ENGINE_TARGET
    eventName = "unity."+eventName;
    if (keyValues != null) {
      Unity_DAS_LogD(eventName, eventValue, keyValues.Keys.ToArray(), keyValues.Values.ToArray(), (uint)keyValues.Count);
    } else {
      Unity_DAS_LogD(eventName, eventValue, null, null, 0);
    }
#endif
  }

  public void Ch_Info(string channelName, string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
#if USE_ENGINE_TARGET
    eventName = "unity."+eventName;
    if (keyValues != null) {
      Unity_DAS_Ch_LogI(channelName, eventName, eventValue, keyValues.Keys.ToArray(), keyValues.Values.ToArray(), (uint)keyValues.Count);
    } else {
      Unity_DAS_Ch_LogI(channelName, eventName, eventValue, null, null, 0);
    }
#endif
  }

  public void Ch_Debug(string channelName, string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
#if USE_ENGINE_TARGET
    eventName = "unity."+eventName;
    if (keyValues != null) {
      Unity_DAS_Ch_LogD(channelName, eventName, eventValue, keyValues.Keys.ToArray(), keyValues.Values.ToArray(), (uint)keyValues.Count);
    } else {
      Unity_DAS_Ch_LogD(channelName, eventName, eventValue, null, null, 0);
    }
#endif
  }

  public void SetGlobal(string key, string value) {
#if USE_ENGINE_TARGET
    Unity_DAS_SetGlobal(key,value);
#endif
  }

#if (UNITY_IOS || UNITY_STANDALONE) && !UNITY_EDITOR
  const string EngineDllName = "__Internal";
#else
  const string EngineDllName = "cozmoEngine";
#endif

#if USE_ENGINE_TARGET
  // TODO: __Internal only works on certain platforms (iOS, but not Android, etc)
  [DllImport(EngineDllName)]
  private static extern void Unity_DAS_Event(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);

  [DllImport(EngineDllName)]
  private static extern void Unity_DAS_LogE(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);

  [DllImport(EngineDllName)]
  private static extern void Unity_DAS_LogW(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);

  [DllImport(EngineDllName)]
  private static extern void Unity_DAS_LogI(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);

  [DllImport(EngineDllName)]
  private static extern void Unity_DAS_LogD(string eventName, string eventValue, string[] keys, string[] values, uint keyValueCount);

  [DllImport(EngineDllName)]
  private static extern void Unity_DAS_Ch_LogI(string channelName, string eventName, string eventValue, string [] keys, string [] values, uint keyValueCount);

  [DllImport(EngineDllName)]
  private static extern void Unity_DAS_Ch_LogD(string channelName, string eventName, string eventValue, string [] keys, string [] values, uint keyValueCount);

  [DllImport(EngineDllName)]
  private static extern void Unity_DAS_SetGlobal(string key, string value);
#endif
}
