using System;

public class UnityDasTarget : IDASTarget {
  #region IDASTarget implementation
  public void Event(string eventName, string eventValue, object context = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 3, eventName, eventValue), context as UnityEngine.Object);
  }
  public void Error(string eventName, string eventValue, object context = null) {
    UnityEngine.Debug.LogError(string.Format("DAS [{0}] {1} - {2}", 5, eventName, eventValue), context as UnityEngine.Object);
  }
  public void Warn(string eventName, string eventValue, object context = null) {
    UnityEngine.Debug.LogWarning(string.Format("DAS [{0}] {1} - {2}", 4, eventName, eventValue), context as UnityEngine.Object);
  }
  public void Info(string eventName, string eventValue, object context = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 2, eventName, eventValue), context as UnityEngine.Object);
  }
  public void Debug(string eventName, string eventValue, object context = null) {
    UnityEngine.Debug.Log(string.Format("DAS [{0}] {1} - {2}", 1, eventName, eventValue), context as UnityEngine.Object);
  }
  #endregion
  
}

