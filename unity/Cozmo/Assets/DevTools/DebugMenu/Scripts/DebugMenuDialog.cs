using UnityEngine;
using UnityEngine.Serialization;
using System.Collections;

[System.Serializable]
public class DebugContentPane{
  public string name;
  public GameObject panePrefab;
}

public class DebugMenuDialog : MonoBehaviour {

  public delegate void DebugMenuDialogHandler(int lastOpenedTab);
  public event DebugMenuDialogHandler OnDebugMenuClosed;

  [SerializeField]
  private DebugContentPane[] debugContentPaneData_;
  
  [SerializeField]
  private DebugMenuDialogTab tabButtonPrefab_;

  [SerializeField]
  private GameObject tabContainer_;

  [SerializeField]
  private GameObject contentPaneContainer_;

  private int currentTab_;
  private GameObject currentPaneObject_;

  public void Initialize(int lastOpenedTab){
    GameObject newTab = null;
    DebugMenuDialogTab newTabScript = null;
    for (int i = 0; i < debugContentPaneData_.Length; i++) {
      newTab = CreateUI(tabButtonPrefab_.gameObject, tabContainer_.transform);
      newTabScript = newTab.GetComponent<DebugMenuDialogTab>();
      newTabScript.Initialize(i, debugContentPaneData_[i].name);
      newTabScript.OnTabTapped += OnTabTapped;
    }

    OpenTab(lastOpenedTab);
  }

	public void OnDebugMenuCloseTap(){
    if (OnDebugMenuClosed != null) {
      OnDebugMenuClosed(currentTab_);
    }
    GameObject.Destroy(gameObject);
  }

  private void OnTabTapped(int tabId){
    if (currentPaneObject_ != null) {
      GameObject.Destroy(currentPaneObject_);
    }
    OpenTab(tabId);
  }

  private void OpenTab(int tabNumber){
    if (tabNumber >= 0 && tabNumber < debugContentPaneData_.Length
        && debugContentPaneData_.Length > 0) {
      currentTab_ = tabNumber;
      currentPaneObject_ = CreateUI(debugContentPaneData_[tabNumber].panePrefab, contentPaneContainer_.transform);
    }
  }

  private GameObject CreateUI(GameObject uiPrefab, Transform uiParent){
    GameObject newUi = GameObject.Instantiate (uiPrefab);
    newUi.transform.SetParent (uiParent, false);
    return newUi;
  }
}
