using System;
using System.Collections.Generic;

public class UnityDasTarget : IDASTarget {
  #region IDASTarget implementation

  public void Event(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 3, eventName, eventValue), context as UnityEngine.Object);
  }

  public void Error(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {
    UnityEngine.Debug.LogError(string.Format("DAS [{0}] {1} - {2}", 5, eventName, eventValue), context as UnityEngine.Object);
  }

  public void Warn(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {
    UnityEngine.Debug.LogWarning(string.Format("DAS [{0}] {1} - {2}", 4, eventName, eventValue), context as UnityEngine.Object);
  }

  public void Info(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 2, eventName, eventValue), context as UnityEngine.Object);
  }

  public void Debug(string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 1, eventName, eventValue), context as UnityEngine.Object);
  }

  public void Ch_Info(string channelName, string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}-{1}] {2} - {3}", 2, channelName, eventName, eventValue), context as UnityEngine.Object);
  }

  public void Ch_Debug(string channelName, string eventName, string eventValue, object context = null, Dictionary<string, string> keyValues = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}-{1}] {2} - {3}", 1, channelName, eventName, eventValue), context as UnityEngine.Object);
  }

  public void SetGlobal(string key, string value) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 6, key, value));
  }

  #endregion
  
}

