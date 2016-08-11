using System;
using System.Collections.Generic;

public interface IDASTarget {
  
  void Event(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null);

  void Error(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null);

  void Warn(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null);

  void Info(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null);

  void Debug(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null);

  void Ch_Info(string channelName, string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null);

  void Ch_Debug(string channelName, string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null);

  void SetGlobal(string key, string value);
}

