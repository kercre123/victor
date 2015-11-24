using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;

namespace Anki.Debug {
  public class DebugConsolePane : MonoBehaviour {

    [SerializeField]
    public RectTransform UIContainer;

    [SerializeField]
    public Text PaneStatusText;

    [SerializeField]
    public GameObject PrefabVarUIText;
    [SerializeField]
    public GameObject PrefabVarUICheckbox;
    [SerializeField]
    public GameObject PrefabVarUIButton;
    [SerializeField]
    public GameObject PrefabVarUISlider;

    [SerializeField]
    public GameObject CategoryPanelPrefab;

    private Dictionary<string, GameObject > _CategoryPanels;

    private void Start() {

      _CategoryPanels = new Dictionary<string, GameObject>();
      DebugConsoleData.Instance.ConsolePane = this;

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
            categoryPanel = UIManager.CreateUIElement(CategoryPanelPrefab, UIContainer);
            _CategoryPanels.Add(categories[i], categoryPanel);
          }

          ConsoleCategoryPanel panelscript = categoryPanel.GetComponent<ConsoleCategoryPanel>();
          panelscript.TitleText.text = categories[i];
          // Add the new field if it exists
          DebugConsoleData.Instance.RefreshCategory(panelscript.UIContainer.transform, categories[i]);
        }
        DebugConsoleData.Instance.OnUIUpdated();
      }
    }


  }

}