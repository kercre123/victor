using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Text;

public class ConsoleLogManager : MonoBehaviour {

  [SerializeField]
  private int numberCachedLogMaximum = 100;

  private Queue<LogPacket> _MostRecentLogs;

  private ConsoleLogPane _ConsoleLogPaneView;

  private Dictionary<LogPacket.ELogKind, bool> _LastToggleValues;

  // Use this for initialization
  private void Awake() {
    _MostRecentLogs = new Queue<LogPacket>();
    _ConsoleLogPaneView = null;

    _LastToggleValues = new Dictionary<LogPacket.ELogKind, bool>();
    _LastToggleValues.Add(LogPacket.ELogKind.INFO, true);
    _LastToggleValues.Add(LogPacket.ELogKind.WARNING, true);
    _LastToggleValues.Add(LogPacket.ELogKind.ERROR, true);
    _LastToggleValues.Add(LogPacket.ELogKind.EVENT, true);
    _LastToggleValues.Add(LogPacket.ELogKind.DEBUG, true);

    DAS.OnInfoLogged += OnInfoLogged;
    DAS.OnWarningLogged += OnWarningLogged;
    DAS.OnErrorLogged += OnErrorLogged;
    DAS.OnDebugLogged += OnDebugLogged;
    DAS.OnEventLogged += OnEventLogged;

    ConsoleLogPane.ConsoleLogPaneOpened += OnConsoleLogPaneOpened;
  }

  private void OnDestroy() {
    DAS.OnInfoLogged -= OnInfoLogged;
    DAS.OnWarningLogged -= OnWarningLogged;
    DAS.OnErrorLogged -= OnErrorLogged;
    DAS.OnDebugLogged -= OnDebugLogged;
    DAS.OnEventLogged -= OnEventLogged;
    ConsoleLogPane.ConsoleLogPaneOpened -= OnConsoleLogPaneOpened;

    if (_ConsoleLogPaneView != null) {
      _ConsoleLogPaneView.ConsoleLogToggleChanged -= OnConsoleToggleChanged;
    }
  }

  private void OnInfoLogged(string eventName, string eventValue, Object context) {
    SaveLogPacket(LogPacket.ELogKind.INFO, eventName, eventValue, context);
  }

  private void OnErrorLogged(string eventName, string eventValue, Object context) {
    SaveLogPacket(LogPacket.ELogKind.ERROR, eventName, eventValue, context);
  }

  private void OnWarningLogged(string eventName, string eventValue, Object context) {
    SaveLogPacket(LogPacket.ELogKind.WARNING, eventName, eventValue, context);
  }

  private void OnEventLogged(string eventName, string eventValue, Object context) {
    SaveLogPacket(LogPacket.ELogKind.EVENT, eventName, eventValue, context);
  }

  private void OnDebugLogged(string eventName, string eventValue, Object context) {
    SaveLogPacket(LogPacket.ELogKind.DEBUG, eventName, eventValue, context);
  }

  private void SaveLogPacket(LogPacket.ELogKind logKind, string eventName, string eventValue, Object context) {
    // Add the new log to the queue
    LogPacket newPacket = new LogPacket(logKind, eventName, eventValue, context);
    _MostRecentLogs.Enqueue(newPacket);
    while (_MostRecentLogs.Count > numberCachedLogMaximum) {
      _MostRecentLogs.Dequeue();
    }
    
    // Update the UI, if it is open
    if (_ConsoleLogPaneView != null) {
      // TODO: Only add the new log to the UI if the filter is toggled on
      if (_LastToggleValues[newPacket.LogKind] == true) {
        _ConsoleLogPaneView.AppendLog(newPacket.ToString());
      }
    }
  }

  private void OnConsoleLogPaneOpened(ConsoleLogPane logPane) {
    _ConsoleLogPaneView = logPane;

    string consoleText = CompileRecentLogs();
    _ConsoleLogPaneView.Initialize(consoleText);
    foreach (KeyValuePair<LogPacket.ELogKind, bool> kvp in _LastToggleValues) {
      _ConsoleLogPaneView.SetToggle(kvp.Key, kvp.Value);
    }
    _ConsoleLogPaneView.ConsoleLogToggleChanged += OnConsoleToggleChanged;
  }

  private void OnConsoleToggleChanged(LogPacket.ELogKind logKind, bool newValue) {
    // IVY: For some reason the newValue is always false - bug on unity's end?
    // lastToggleValues_[logKind] = newValue;
    _LastToggleValues[logKind] = !_LastToggleValues[logKind];

    // Change the text for the pane
    string consoleText = CompileRecentLogs();
    _ConsoleLogPaneView.SetText(consoleText);
  }

  private void OnConsoleLogPaneClosed() {
    _ConsoleLogPaneView.ConsoleLogToggleChanged -= OnConsoleToggleChanged;
    _ConsoleLogPaneView = null;
  }

  private string CompileRecentLogs() {
    StringBuilder sb = new StringBuilder();
    foreach (LogPacket packet in _MostRecentLogs) {
      if (_LastToggleValues[packet.LogKind] == true) {
        sb.Append(packet.ToString());
        sb.Append("\n");
      }
    }
    return sb.ToString();
  }
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