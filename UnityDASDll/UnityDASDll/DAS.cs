using System;
using System.Runtime.InteropServices;
using UnityEngine;

public interface IDAS {
  void Event(string eventValue, UnityEngine.Object context = null);

  void Error(string eventValue, UnityEngine.Object context = null);

  void Warn(string eventValue, UnityEngine.Object context = null);

  void Info(string eventValue, UnityEngine.Object context = null);

  void Debug(string eventValue, UnityEngine.Object context = null);
}

public static partial class DAS {

  public delegate void DASHandler(string eventName, string eventValue, UnityEngine.Object context);
  public static event DASHandler OnEventLogged;
  public static event DASHandler OnErrorLogged;
  public static event DASHandler OnWarningLogged;
  public static event DASHandler OnInfoLogged;
  public static event DASHandler OnDebugLogged;

  public static void Event(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 3, eventName, eventValue), context);
    if (OnEventLogged != null) {
      OnEventLogged(eventName, eventValue, context);
    }
  }

  public static void Error(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
    UnityEngine.Debug.LogError(string.Format("DAS [{0}] {1} - {2}", 5, eventName, eventValue), context);
    if (OnErrorLogged != null) {
      OnErrorLogged(eventName, eventValue, context);
    }
  }

  public static void Warn(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
    UnityEngine.Debug.LogWarning(string.Format("DAS [{0}] {1} - {2}", 4, eventName, eventValue), context);
    if (OnWarningLogged != null) {
      OnWarningLogged(eventName, eventValue, context);
    }
  }

  public static void Info(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 2, eventName, eventValue), context);
    if (OnInfoLogged != null) {
      OnInfoLogged(eventName, eventValue, context);
    }
  }

  public static void Debug(object eventObject, string eventValue, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 1, eventName, eventValue), context);
    if (OnDebugLogged != null) {
      OnDebugLogged(eventName, eventValue, context);
    }
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
  
}
