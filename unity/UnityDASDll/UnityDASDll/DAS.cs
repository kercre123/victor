using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

public interface IDAS {
  void Event(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Error(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Error(Exception eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Warn(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Info(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Debug(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Ch_Info(string channelName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Ch_Debug(string channelName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void SetGlobal(string key, string value);
}

public static partial class DAS {

  private static readonly List<IDASTarget> _Targets = new List<IDASTarget>();

  public static void AddTarget(IDASTarget target) {
    _Targets.Add(target);
  }

  public static void RemoveTarget(IDASTarget target) {
    _Targets.Remove(target);
  }

  public static void ClearTargets() {
    _Targets.Clear();
  }

  public static void Event(object eventObject, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);

    for (int i = 0, len = _Targets.Count; i < len; i++) {
      _Targets[i].Event(eventName, eventValue, keyValues, context);
    }
  }

  public static void Error(object eventObject, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);

    for (int i = 0, len = _Targets.Count; i < len; i++) {
      _Targets[i].Error(eventName, eventValue, keyValues, context);
    }
  }

  public static void Error(object eventObject, Exception eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);

    for (int i = 0, len = _Targets.Count; i < len; i++) {
      _Targets[i].Error(eventName, eventValue.Message + "\n" + eventValue.StackTrace, keyValues, context);
    }
  }

  public static void Warn(object eventObject, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);

    for (int i = 0, len = _Targets.Count; i < len; i++) {
      _Targets[i].Warn(eventName, eventValue, keyValues, context);
    }
  }

  public static void Info(object eventObject, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);

    for (int i = 0, len = _Targets.Count; i < len; i++) {
      _Targets[i].Info(eventName, eventValue, keyValues, context);
    }
  }

  public static void Debug(object eventObject, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    string eventName = GetEventName(eventObject);

    for (int i = 0, len = _Targets.Count; i < len; i++) {
      _Targets[i].Debug(eventName, eventValue, keyValues, context);
    }
  }

  public static void Ch_Info(string channelName, object eventObject, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    string eventName = GetEventName (eventObject);
    for (int i = 0, len = _Targets.Count; i < len; i++) {
      _Targets[i].Ch_Info(channelName, eventName, eventValue, keyValues, context);
    }
  }

  public static void Ch_Debug(string channelName, object eventObject, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
    string eventName = GetEventName (eventObject);
    for (int i = 0, len = _Targets.Count; i < len; i++) {
      _Targets[i].Ch_Debug(channelName, eventName, eventValue, keyValues, context);
    }
  }

  public static void SetGlobal(string key, string value) {
    for (int i = 0, len = _Targets.Count; i < len; i++) {
      _Targets[i].SetGlobal(key, value);
    }
  }

  private static string GetEventName(object eventObject) {
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

  private sealed class DASInstance : IDAS {
    private string _EventName;

    public DASInstance(System.Type type) {
      _EventName = type.Name;
    }

    public void Event(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
      DAS.Event(_EventName, eventValue, keyValues, context);
    }

    public void Error(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
      DAS.Error(_EventName, eventValue, keyValues, context);      
    }

    public void Error(Exception eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
      DAS.Error(_EventName, eventValue, keyValues, context);      
    }

    public void Warn(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
      DAS.Warn(_EventName, eventValue, keyValues, context);      
    }

    public void Info(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
      DAS.Info(_EventName, eventValue, keyValues, context);      
    }

    public void Debug(string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
      DAS.Debug(_EventName, eventValue, keyValues, context);            
    }

    public void Ch_Info(string channelName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
      DAS.Ch_Info(channelName, _EventName, eventValue, keyValues, context);
    }
    
    public void Ch_Debug(string channelName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null) {
      DAS.Ch_Debug(channelName, _EventName, eventValue, keyValues, context);
    }

    public void SetGlobal(string key, string value) {
      DAS.SetGlobal(key, value);            
    }
  }
  
}
