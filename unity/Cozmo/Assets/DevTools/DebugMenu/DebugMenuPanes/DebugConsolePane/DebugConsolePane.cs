using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;
using System.Collections.Generic;

public class DebugConsolePane : MonoBehaviour {

  [SerializeField]
  private RectTransform _UIContainer;

  [SerializeField]
  private GameObject _PrefabVarUIText;
  [SerializeField]
  private GameObject _PrefabVarUICheckbox;
  [SerializeField]
  private GameObject _PrefabVarUIButton;
  [SerializeField]
  private GameObject _PrefabVarUISlider;

  [SerializeField]
  private GameObject _CategoryPanelPrefab;

  private void Start() {
    // Query for our initial data so DebugConsoleData gets populated when dirty in update.
    DAS.Info("RobotSettingsPane", "INITTING!");
    RobotEngineManager.Instance.InitDebugConsole();

    DebugConsoleData.Instance._PrefabVarUIText = _PrefabVarUIText.gameObject;
    DebugConsoleData.Instance._PrefabVarUICheckbox = _PrefabVarUICheckbox.gameObject;
    DebugConsoleData.Instance._PrefabVarUIButton = _PrefabVarUIButton.gameObject;
    DebugConsoleData.Instance._PrefabVarUISlider = _PrefabVarUISlider.gameObject;
  }

  void Update() {
    // if the static class is up, do a refresh of data.
    if (DebugConsoleData.Instance.NeedsUIUpdate()) {

      List<string> categories = DebugConsoleData.Instance.GetCategories();
      for (int i = 0; i < categories.Count; ++i) {
        GameObject categoryPanel = UIManager.CreateUIElement(_CategoryPanelPrefab, _UIContainer);

        ConsoleCategoryPanel panelscript = categoryPanel.GetComponent<ConsoleCategoryPanel>();
        panelscript._TitleText.text = categories[i];
        DebugConsoleData.Instance.AddCategory(panelscript._UIContainer.transform, categories[i]);
      }
      DebugConsoleData.Instance.OnUIUpdated();
    }
  }


}
