using System;
using System.Runtime.InteropServices;
using UnityEngine;

#if (!UNITY_EDITOR && (UNITY_ANDROID || UNITY_IPHONE || UNITY_STANDALONE)) || ANIMATION_TOOL

public interface IDAS {
  void Event(string eventValue, UnityEngine.Object context = null);

  void Error(string eventValue, UnityEngine.Object context = null);

  void Warn(string eventValue, UnityEngine.Object context = null);

  void Info(string eventValue, UnityEngine.Object context = null);

  void Debug(string eventValue, UnityEngine.Object context = null);
}

public static class DAS {

  public delegate void DASHandler(string eventName, string eventValue, UnityEngine.Object context);
  public static event DASHandler OnEventLogged;
  public static event DASHandler OnErrorLogged;
  public static event DASHandler OnWarningLogged;
  public static event DASHandler OnInfoLogged;
  public static event DASHandler OnDebugLogged;

  public static void Event(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    UnitysDAS_Event(eventName, eventValue);
    if (OnEventLogged != null){
      OnEventLogged(eventName, eventValue, context);
    }
#endif
  }
 
  public static void Error(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    UnitysDAS_LogE(eventName, eventValue);
    if (OnErrorLogged != null){
      OnErrorLogged(eventName, eventValue, context);
    }
#endif
  }

  public static void Warn(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
#if ANIMATION_TOOL
      Console.Out.WriteLine(eventName + ": " + eventValue);

#else
      UnitysDAS_LogW(eventName, eventValue);
    if (OnWarningLogged != null){
      OnWarningLogged(eventName, eventValue, context);
    }
#endif
    }

  public static void Info(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    UnitysDAS_LogI(eventName, eventValue);
    if (OnInfoLogged != null){
      OnInfoLogged(eventName, eventValue, context);
    }
#endif
  }

  public static void Debug(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
#if ANIMATION_TOOL
    Console.Out.WriteLine(eventName + ": " + eventValue);

#else
    UnitysDAS_LogD(eventName, eventValue);
    if (OnDebugLogged != null){
      OnDebugLogged(eventName, eventValue, context);
    }
#endif
  }

  private static string GetEventName(object eventObject)
  {
    if (eventObject == null) {
      return string.Empty;
    }
    var eventString = eventObject as string;
    if (eventString != null) {
      return eventString;
    }

    var eventType = eventObject as System.Type;
    if (eventType != null) {
      return eventType.Name;
    }

    return eventObject.GetType().Name;
  }

  public static IDAS GetInstance(System.Type type) {
    // this could be modified to do a dictionary lookup if it was a concern
    // that multiple instance for the same type would be retrieved
    return new DASInstance(type);
  }

  private sealed class DASInstance : IDAS
  {
    private string _EventName;

    public DASInstance(System.Type type) {
      _EventName = type.Name;
    }

    public void Event(string eventValue, UnityEngine.Object context = null) {
      DAS.Event(_EventName, eventValue, context);
    }

    public void Error(string eventValue, UnityEngine.Object context = null) {
      DAS.Error(_EventName, eventValue, context);      
    }

    public void Warn(string eventValue, UnityEngine.Object context = null) {
      DAS.Warn(_EventName, eventValue, context);      
    }

    public void Info(string eventValue, UnityEngine.Object context = null) {
      DAS.Info(_EventName, eventValue, context);      
    }

    public void Debug(string eventValue, UnityEngine.Object context = null) {
      DAS.Debug(_EventName, eventValue, context);            
    }
  }


  // TODO: __Internal only works on certain platforms (iOS, but not Android, etc)
  [DllImport("__Internal")]
  private static extern void UnitysDAS_Event(string eventName, string eventValue);

  [DllImport("__Internal")]
  private static extern void UnitysDAS_LogE(string eventName, string eventValue);

  [DllImport("__Internal")]
  private static extern void UnitysDAS_LogW(string eventName, string eventValue);

  [DllImport("__Internal")]
  private static extern void UnitysDAS_LogI(string eventName, string eventValue);

  [DllImport("__Internal")]
  private static extern void UnitysDAS_LogD(string eventName, string eventValue);
}
#endif