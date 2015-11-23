using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;

namespace Anki.Debug {
  public class DebugConsolePane : MonoBehaviour {

    [SerializeField]
    public RectTransform _UIContainer;

    [SerializeField]
    public Text _PaneStatusText;

    [SerializeField]
    public GameObject _PrefabVarUIText;
    [SerializeField]
    public GameObject _PrefabVarUICheckbox;
    [SerializeField]
    public GameObject _PrefabVarUIButton;
    [SerializeField]
    public GameObject _PrefabVarUISlider;

    [SerializeField]
    public GameObject _CategoryPanelPrefab;

    private Dictionary<string, GameObject > _CategoryPanels;

    private void Start() {

      _CategoryPanels = new Dictionary<string, GameObject>();
      DebugConsoleData.Instance._ConsolePane = this;

      // Query for our initial data so DebugConsoleData gets populated when dirty in update.
      RobotEngineManager.Instance.InitDebugConsole();
    }

    void Update() {
      // if the static class is up, do a refresh of data.
      if (DebugConsoleData.Instance.NeedsUIUpdate()) {
        List<string> categories = DebugConsoleData.Instance.GetCategories();
        for (int i = 0; i < categories.Count; ++i) {
          // Get existing child if it exists, otherwise create it.
          GameObject categoryPanel = null;
          if (!_CategoryPanels.TryGetValue(categories[i], out  categoryPanel)) {
            categoryPanel = UIManager.CreateUIElement(_CategoryPanelPrefab, _UIContainer);
            _CategoryPanels.Add(categories[i], categoryPanel);
          }

          ConsoleCategoryPanel panelscript = categoryPanel.GetComponent<ConsoleCategoryPanel>();
          panelscript._TitleText.text = categories[i];
          // Add the new field if it exists
          DebugConsoleData.Instance.RefreshCategory(panelscript._UIContainer.transform, categories[i]);
        }
        DebugConsoleData.Instance.OnUIUpdated();
      }
    }


  }

}