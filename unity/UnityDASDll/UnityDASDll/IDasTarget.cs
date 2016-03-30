using System;

public interface IDASTarget {
  
  void Event(string eventName, string eventValue, object context = null);

  void Error(string eventName, string eventValue, object context = null);

  void Warn(string eventName, string eventValue, object context = null);

  void Info(string eventName, string eventValue, object context = null);

  void Debug(string eventName, string eventValue, object context = null);
}

