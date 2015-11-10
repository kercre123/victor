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

  private int _CurrentTab;
  private GameObject _CurrentPaneObject;

  public void Initialize(int lastOpenedTab) {
    GameObject newTab = null;
    DebugMenuDialogTab newTabScript = null;
    for (int i = 0; i < _DebugContentPaneData.Length; i++) {
      newTab = CreateUI(_TabButtonPrefab.gameObject, _TabContainer.transform);
      newTabScript = newTab.GetComponent<DebugMenuDialogTab>();
      newTabScript.Initialize(i, _DebugContentPaneData[i].name);
      newTabScript.OnTabTapped += OnTabTapped;
    }

    OpenTab(lastOpenedTab);
  }

  public void OnDebugMenuCloseTap() {
    if (OnDebugMenuClosed != null) {
      OnDebugMenuClosed(_CurrentTab);
    }
    GameObject.Destroy(gameObject);
  }

  private void OnTabTapped(int tabId) {
    if (_CurrentPaneObject != null) {
      GameObject.Destroy(_CurrentPaneObject);
    }
    OpenTab(tabId);
  }

  private void OpenTab(int tabNumber) {
    if (tabNumber >= 0 && tabNumber < _DebugContentPaneData.Length
        && _DebugContentPaneData.Length > 0) {
      _CurrentTab = tabNumber;
      _CurrentPaneObject = CreateUI(_DebugContentPaneData[tabNumber].panePrefab, _ContentPaneContainer.transform);
    }
  }

  private GameObject CreateUI(GameObject uiPrefab, Transform uiParent) {
    GameObject newUi = GameObject.Instantiate(uiPrefab);
    newUi.transform.SetParent(uiParent, false);
    return newUi;
  }
}
