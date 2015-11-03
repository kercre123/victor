using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Text;

public class ConsoleLogManager : MonoBehaviour {

  [SerializeField]
  private int numberCachedLogMaximum = 100;

  private Queue<LogPacket> mostRecentLogs_;

  private ConsoleLogPane consoleLogPaneView_;

	// Use this for initialization
	private void Awake () {
    mostRecentLogs_ = new Queue<LogPacket>();
    consoleLogPaneView_ = null;

    DAS.OnInfoLogged += OnInfoLogged;
    DAS.OnWarningLogged += OnWarningLogged;
    DAS.OnErrorLogged += OnErrorLogged;
    DAS.OnDebugLogged += OnDebugLogged;
    DAS.OnEventLogged += OnEventLogged;

    ConsoleLogPane.ConsoleLogPaneOpened += OnConsoleLogPaneOpened;
	}

  private void OnDestroy(){
    DAS.OnInfoLogged -= OnInfoLogged;
    DAS.OnWarningLogged -= OnWarningLogged;
    DAS.OnErrorLogged -= OnErrorLogged;
    DAS.OnDebugLogged -= OnDebugLogged;
    DAS.OnEventLogged -= OnEventLogged;
    ConsoleLogPane.ConsoleLogPaneOpened -= OnConsoleLogPaneOpened;
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

  private void OnConsoleLogPaneOpened(ConsoleLogPane logPane){
    consoleLogPaneView_ = logPane;

    StringBuilder sb = new StringBuilder();
    foreach (LogPacket packet in mostRecentLogs_) {
      sb.Append(packet.ToString());
      sb.Append("\n");
    }

    consoleLogPaneView_.Initialize(sb.ToString());
  }

  private void OnConsoleLogPaneClosed(){
    // TODO: Save last used filters
    consoleLogPaneView_ = null;
  }

  private void SaveLogPacket(LogPacket.ELogKind logKind, string eventName, string eventValue, Object context){
    // Add the new log to the queue
    LogPacket newPacket = new LogPacket(logKind, eventName, eventValue, context);
    mostRecentLogs_.Enqueue(newPacket);
    while (mostRecentLogs_.Count > numberCachedLogMaximum) {
      mostRecentLogs_.Dequeue();
    }

    // Update the UI, if it is open
    if (consoleLogPaneView_ != null) {
      // TODO: Only add the new log to the UI if the filter is toggled on
      consoleLogPaneView_.AppendLog(newPacket.ToString());
    }
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

  public override string ToString() {
    string logKindStr = "";
    string colorStr = "";
    switch (LogKind) {
    case ELogKind.INFO:
      logKindStr = "INFO";
      colorStr = "ffffff";
      break;
    case ELogKind.WARNING:
      logKindStr = "WARN";
      colorStr = "ffcc00";
      break;
    case ELogKind.ERROR:
      logKindStr = "ERROR";
      colorStr = "ff0000";
      break;
    case ELogKind.EVENT:
      logKindStr = "EVENT";
      colorStr = "0099ff";
      break;
    case ELogKind.DEBUG:
      logKindStr = "DEBUG";
      colorStr = "00cc00";
      break;
    }
    
    string contextStr = "null";
    if (Context != null) {
      contextStr = Context.ToString();
    }
    // TODO: Colorize the text
    return string.Format("<color=#{0}>[{1}] {2}: {3} ({4})</color>", colorStr, logKindStr, EventName, EventValue, contextStr);
  }
}