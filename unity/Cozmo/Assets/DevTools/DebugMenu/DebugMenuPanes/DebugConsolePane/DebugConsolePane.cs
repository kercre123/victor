using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class DebugConsolePane : MonoBehaviour {

  [SerializeField]
  private RectTransform _UIContainer;

  [SerializeField]
  private ConsoleVarLine _SingleLinePrefab;

  private void Start() {

    // Query for our initial data so DebugConsoleData gets populated when dirty in update.
    DAS.Info("RobotSettingsPane", "INITTING!");
    RobotEngineManager.Instance.InitDebugConsole();
  }

  void Update() {
    // if the static class is up, do a refresh of data.
    if (DebugConsoleData.Instance.NeedsUIUpdate()) {
      int count = DebugConsoleData.Instance.GetCountVars();
      // TODO: sort by categories.
      for (int i = 0; i < count; ++i) {
        DebugConsoleData.DebugConsoleVarData singleVar = DebugConsoleData.Instance.GetDataAtIndex(i);

        GameObject statLine = UIManager.CreateUI(_SingleLinePrefab.gameObject, _UIContainer);
        ConsoleVarLine uiLine = statLine.GetComponent<ConsoleVarLine>();
        uiLine.Init(singleVar);
      }
      DebugConsoleData.Instance.OnUIUpdated();
    }
  }


}
