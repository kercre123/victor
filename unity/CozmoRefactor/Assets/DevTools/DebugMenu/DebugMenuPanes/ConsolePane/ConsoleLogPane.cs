using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Text;

public class ConsoleLogPane : MonoBehaviour {

  public delegate void ConsoleLogPaneOpenHandler(ConsoleLogPane consoleLogPane);
  public static event ConsoleLogPaneOpenHandler ConsoleLogPaneOpened;
  private static void RaiseConsoleLogPaneOpened(ConsoleLogPane consoleLogPane){
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

  [SerializeField]
  private Text consoleTextLabel_;

  [SerializeField]
  private ScrollRect textScrollRect_;

	// Use this for initialization
	private void Start () {
    RaiseConsoleLogPaneOpened(this);
	}

  private void OnDestroy() {
    RaiseConsoleLogPaneClosed();
  }

  public void Initialize(string consoleText) {
    consoleTextLabel_.text = consoleText;
    textScrollRect_.verticalNormalizedPosition = 1;
  }

  public void AppendLog(string newLog) {
    bool wasAtBottom = (textScrollRect_.verticalNormalizedPosition >= 1);
    StringBuilder sb = new StringBuilder();
    sb.Append(consoleTextLabel_.text);
    sb.Append(newLog);
    sb.Append("\n");
    consoleTextLabel_.text = sb.ToString();

    if (wasAtBottom) {
      textScrollRect_.verticalNormalizedPosition = 1;
    }
  }
}
