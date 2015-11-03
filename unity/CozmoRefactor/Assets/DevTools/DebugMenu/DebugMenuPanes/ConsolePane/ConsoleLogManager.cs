using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class ConsoleLogManager : MonoBehaviour {

  [SerializeField]
  private int numberCachedLogMaximum = 100;

  private Queue<LogPacket> mostRecentLogs_;

	// Use this for initialization
	private void Awake () {
    mostRecentLogs_ = new Queue<LogPacket>();

    DAS.OnInfoLogged += OnInfoLogged;
    DAS.OnWarningLogged += OnWarningLogged;
    DAS.OnErrorLogged += OnErrorLogged;
    DAS.OnDebugLogged += OnDebugLogged;
    DAS.OnEventLogged += OnEventLogged;
	}

  private void OnDestroy(){
    DAS.OnInfoLogged -= OnInfoLogged;
    DAS.OnWarningLogged -= OnWarningLogged;
    DAS.OnErrorLogged -= OnErrorLogged;
    DAS.OnDebugLogged -= OnDebugLogged;
    DAS.OnEventLogged -= OnEventLogged;
  }

  private void OnInfoLogged(string eventName, string eventValue, Object context){
    SaveLogPacket(LogPacket.ELogKind.INFO, eventName, eventValue, context);
  }

  private void OnErrorLogged(string eventName, string eventValue, Object context){
    SaveLogPacket(LogPacket.ELogKind.ERROR, eventName, eventValue, context);
  }

  private void OnWarningLogged(string eventName, string eventValue, Object context){
    SaveLogPacket(LogPacket.ELogKind.WARNING, eventName, eventValue, context);
  }

  private void OnEventLogged(string eventName, string eventValue, Object context){
    SaveLogPacket(LogPacket.ELogKind.EVENT, eventName, eventValue, context);
  }

  private void OnDebugLogged(string eventName, string eventValue, Object context){
    SaveLogPacket(LogPacket.ELogKind.DEBUG, eventName, eventValue, context);
  }

  private void SaveLogPacket(LogPacket.ELogKind logKind, string eventName, string eventValue, Object context){
    // Add the new log to the queue
    LogPacket newPacket = new LogPacket(logKind, eventName, eventValue, context);
    mostRecentLogs_.Enqueue(newPacket);
    while (mostRecentLogs_.Count > numberCachedLogMaximum) {
      mostRecentLogs_.Dequeue();
    }

    // TODO: Update the UI, if it is open
    // TODO: Only add the new log to the UI if the filter is toggled on
  }

  // TODO: Pragma out for production
  #region TESTING
  public void OnAddInfoTap(){
    DAS.Info("DebugMenuManager", "Manually Adding Info Info Info");
  }
  
  public void OnAddWarningTap(){
    DAS.Warn("DebugMenuManager", "Manually Adding Warn Warn Warn");
  }
  
  public void OnAddErrorTap(){
    DAS.Error("DebugMenuManager", "Manually Adding Error Error Error");
  }
  
  public void OnAddEventTap(){
    DAS.Event("DebugMenuManager", "Manually Adding Event Event Event");
  }
  
  public void OnAddDebugTap(){
    DAS.Debug("DebugMenuManager", "Manually Adding Debug Debug Debug");
  }
  #endregion
}

public class LogPacket {
  public enum ELogKind {
    INFO,
    WARNING,
    ERROR,
    DEBUG,
    EVENT
  }
  
  public ELogKind LogKind {
    get;
    private set;
  }
  public string EventName {
    get;
    private set;
  }
  public string EventValue {
    get;
    private set;
  }
  public Object Context {
    get;
    private set;
  }
  
  public LogPacket(ELogKind logKind, string eventName, string eventValue, Object context) {
    LogKind = logKind;
    EventName = eventName;
    EventValue = eventValue;
    Context = context;
  }
}