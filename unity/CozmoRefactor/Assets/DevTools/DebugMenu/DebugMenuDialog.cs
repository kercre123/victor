using UnityEngine;
using UnityEngine.Serialization;
using System.Collections;

[System.Serializable]
public class DebugContentPane{
  public string name;
  public GameObject panePrefab;
}

public class DebugMenuDialog : MonoBehaviour {

  [SerializeField]
  private DebugContentPane[] debugContentPaneData_;
  
  [SerializeField]
  private GameObject tabButtonPrefab_;

  [SerializeField]
  private GameObject tabContainer_;

  [SerializeField]
  private GameObject contentPaneContainer_;

  private int currentTab;
  private GameObject currentPaneObject;

  public void Initialize(int lastOpenedTab){
    foreach (DebugContentPane pane in debugContentPaneData_) {
      // TODO: Hook up the buttons to switch tabs
      // TODO: Hook up names to the tabs
      CreateUI(tabButtonPrefab_, tabContainer_.transform);
    }

    OpenTab(lastOpenedTab);
  }

	public void OnDebugMenuCloseTap(){
    // TODO Save the currently opened tab before closing the dialog
    GameObject.Destroy(gameObject);
  }

  private void OpenTab(int tabNumber){
    if (tabNumber >= 0 && tabNumber < debugContentPaneData_.Length
        && debugContentPaneData_.Length > 0) {
      currentTab = tabNumber;
      currentPaneObject = CreateUI(debugContentPaneData_[tabNumber].panePrefab, contentPaneContainer_.transform);
    }
  }

  private GameObject CreateUI(GameObject uiPrefab, Transform uiParent){
    GameObject newUi = GameObject.Instantiate (uiPrefab);
    newUi.transform.SetParent (uiParent, false);
    return newUi;
  }
}
