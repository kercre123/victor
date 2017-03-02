using UnityEngine;
using UnityEngine.Serialization;
using System.Collections;

[System.Serializable]
public class DebugContentPane {
  public string name;
  public GameObject panePrefab;
}

public class DebugMenuDialog : MonoBehaviour {

  public delegate void DebugMenuDialogHandler(int lastOpenedTab);

  public event DebugMenuDialogHandler OnDebugMenuClosed;

  [SerializeField]
  private DebugContentPane[] _DebugContentPaneData;

  [SerializeField]
  private DebugMenuDialogTab _TabButtonPrefab;

  [SerializeField]
  private GameObject _TabContainer;

  [SerializeField]
  private GameObject _ContentPaneContainer;

  [SerializeField]
  private UnityEngine.UI.ScrollRect _MenuScrollRect;

  [SerializeField]
  private GameObject _ContentPaneWrapperPrefab;
  private GameObject _ContentPaneWrapper = null;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _BackButtonPrefab;
  private Cozmo.UI.CozmoButtonLegacy _BackButtonInstance = null;

  private int _CurrentTab;
  private GameObject _CurrentPaneObject;

  public void Initialize(int lastOpenedTab) {
    GameObject newTab = null;
    DebugMenuDialogTab newTabScript = null;

    System.Array.Sort(_DebugContentPaneData, delegate (DebugContentPane pane1, DebugContentPane pane2) {
      return pane1.name.CompareTo(pane2.name);
    });

    for (int i = 0; i < _DebugContentPaneData.Length; i++) {
      newTab = CreateUI(_TabButtonPrefab.gameObject, _TabContainer.transform);
      newTabScript = newTab.GetComponent<DebugMenuDialogTab>();
      newTabScript.Initialize(i, _DebugContentPaneData[i].name);
      newTabScript.OnTabTapped += OnTabTapped;
    }
    _MenuScrollRect.verticalNormalizedPosition = 1.0f;
  }

  public void OnDebugMenuCloseTap() {
    if (OnDebugMenuClosed != null) {
      OnDebugMenuClosed(_CurrentTab);
    }
    GameObject.Destroy(gameObject);
  }

  private void OnTabTapped(int tabId) {
    CloseTab();
    OpenTab(tabId);
  }

  private void OpenTab(int tabNumber) {
    if (tabNumber >= 0 && tabNumber < _DebugContentPaneData.Length && _DebugContentPaneData.Length > 0) {
      _CurrentTab = tabNumber;
      _ContentPaneWrapper = CreateUI(_ContentPaneWrapperPrefab, _ContentPaneContainer.transform);
      _CurrentPaneObject = CreateUI(_DebugContentPaneData[tabNumber].panePrefab, _ContentPaneContainer.transform);
      _BackButtonInstance = CreateUI(_BackButtonPrefab.gameObject, _ContentPaneContainer.transform).GetComponent<Cozmo.UI.CozmoButtonLegacy>();
      _BackButtonInstance.Initialize(CloseTab, "debug_menu_back_button", "debug_menu");
    }
  }

  private void CloseTab() {
    if (_CurrentPaneObject != null) {
      // Destroy immediate so the current pane's OnDestroy is called before the new pane's Start.
      GameObject.DestroyImmediate(_CurrentPaneObject);
    }
    if (_ContentPaneWrapper != null) {
      GameObject.DestroyImmediate(_ContentPaneWrapper);
    }
    if (_BackButtonInstance != null) {
      GameObject.DestroyImmediate(_BackButtonInstance.gameObject);
    }
  }

  private GameObject CreateUI(GameObject uiPrefab, Transform uiParent) {
    GameObject newUi = GameObject.Instantiate(uiPrefab);
    newUi.transform.SetParent(uiParent, false);
    return newUi;
  }
}
