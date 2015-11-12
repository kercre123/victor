using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Text;

public class ConsoleLogPane : MonoBehaviour {

  public delegate void ConsoleLogPaneOpenHandler(ConsoleLogPane consoleLogPane);

  public static event ConsoleLogPaneOpenHandler ConsoleLogPaneOpened;

  private static void RaiseConsoleLogPaneOpened(ConsoleLogPane consoleLogPane) {
    if (ConsoleLogPaneOpened != null) {
      ConsoleLogPaneOpened(consoleLogPane);
    }
  }

  public delegate void ConsoleLogPaneClosedHandler();

  public event ConsoleLogPaneClosedHandler ConsoleLogPaneClosed;

  private void RaiseConsoleLogPaneClosed() {
    if (ConsoleLogPaneClosed != null) {
      ConsoleLogPaneClosed();
    }
  }

  public delegate void ConsoleLogToggleChangedHandler(LogPacket.ELogKind logKind, bool newIsOnValue);

  public event ConsoleLogToggleChangedHandler ConsoleLogToggleChanged;

  private void RaiseConsoleLogToggleChanged(LogPacket.ELogKind logKind, bool newIsOnValue) {
    if (ConsoleLogToggleChanged != null) {
      ConsoleLogToggleChanged(logKind, newIsOnValue);
    }
  }

  [SerializeField]
  private Text _ConsoleTextLabel;

  [SerializeField]
  private ScrollRect _TextScrollRect;
  
  [SerializeField]
  private ConsoleLogToggle[] _LogToggles;

  // Use this for initialization
  private void Start() {
    RaiseConsoleLogPaneOpened(this);
  }

  private void OnDestroy() {
    RaiseConsoleLogPaneClosed();
  }

  public void Initialize(string consoleText) {
    _ConsoleTextLabel.text = consoleText;
    _TextScrollRect.verticalNormalizedPosition = 1;
  }

  public void SetText(string consoleText) {
    bool wasAtBottom = (_TextScrollRect.verticalNormalizedPosition >= 1);
    _ConsoleTextLabel.text = consoleText;
    if (wasAtBottom) {
      _TextScrollRect.verticalNormalizedPosition = 1;
    }
  }

  public void AppendLog(string newLog) {
    bool wasAtBottom = (_TextScrollRect.verticalNormalizedPosition >= 1);
    StringBuilder sb = new StringBuilder();
    sb.Append(_ConsoleTextLabel.text);
    sb.Append(newLog);
    sb.Append("\n");
    _ConsoleTextLabel.text = sb.ToString();

    if (wasAtBottom) {
      _TextScrollRect.verticalNormalizedPosition = 1;
    }
  }

  public void SetToggle(LogPacket.ELogKind logKind, bool isOn) {
    foreach (ConsoleLogToggle logToggle in _LogToggles) {
      if (logToggle.logKind == logKind) {
        logToggle.toggle.isOn = isOn;
      }
    }
  }

  public void OnInfoToggleChanged(bool newValue) {
    RaiseConsoleLogToggleChanged(LogPacket.ELogKind.INFO, newValue);
  }

  public void OnWarningToggleChanged(bool newValue) {
    RaiseConsoleLogToggleChanged(LogPacket.ELogKind.WARNING, newValue);
  }

  public void OnErrorToggleChanged(bool newValue) {
    RaiseConsoleLogToggleChanged(LogPacket.ELogKind.ERROR, newValue);
  }

  public void OnEventToggleChanged(bool newValue) {
    RaiseConsoleLogToggleChanged(LogPacket.ELogKind.EVENT, newValue);
  }

  public void OnDebugToggleChanged(bool newValue) {
    RaiseConsoleLogToggleChanged(LogPacket.ELogKind.DEBUG, newValue);
  }
}

[System.Serializable]
public class ConsoleLogToggle {
  public LogPacket.ELogKind logKind;
  public Toggle toggle;
}