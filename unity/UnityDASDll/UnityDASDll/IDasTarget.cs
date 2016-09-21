using System;
using System.Collections.Generic;

public interface IDASTarget {
  
  void Event(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Error(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Warn(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Info(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Debug(string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Ch_Info(string channelName, string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void Ch_Debug(string channelName, string eventName, string eventValue, Dictionary<string, string> keyValues = null, UnityEngine.Object context = null);

  void SetGlobal(string key, string value);
}

