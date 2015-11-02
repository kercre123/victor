using UnityEngine;
using System.Collections;

public class DebugMenuManager : MonoBehaviour {

	[SerializeField]
	private DebugMenuDialog debugMenuDialogPrefab_;
  private DebugMenuDialog debugMenuDialogInstance_;

  [SerializeField]
  private Canvas debugMenuCanvas_;

  private int lastOpenedDebugTab_ = 0;

  void Start(){
    // TODO: Destroy self if not production
  }
  
  // TODO: Pragma out this code for production
  public void OnDebugMenuButtonTap(){
    CreateDebugDialog();
  }
  
  private void CreateDebugDialog(){
    GameObject debugMenuDialogInstance = GameObject.Instantiate(debugMenuDialogPrefab_.gameObject);
    Transform dialogTransform = debugMenuDialogInstance.transform;
    dialogTransform.SetParent(debugMenuCanvas_.transform, false);
    dialogTransform.localPosition = Vector3.zero;
    debugMenuDialogInstance_ = debugMenuDialogInstance.GetComponent<DebugMenuDialog>();
    debugMenuDialogInstance_.Initialize(lastOpenedDebugTab_);
    debugMenuDialogInstance_.OnDebugMenuClosed += OnDebugMenuDialogClose;
  }

  private void OnDebugMenuDialogClose(int lastOpenTab){
    debugMenuDialogInstance_.OnDebugMenuClosed -= OnDebugMenuDialogClose;
    lastOpenedDebugTab_ = lastOpenTab;
  }

  private void OnAddInfo(string eventName, string eventValue){
  }
 
  #region TESTING
	public void OnAddInfoTap(){
    DAS.Info("DebugMenuManager", "Manually Adding Info Info Info");
	}

	public void OnAddWarningTap(){
    DAS.Warn("DebugMenuManager", "Manually Adding Warn Warn Warn");
	}

	public void OnAddErrorTap(){
    DAS.Error("DebugMenuManager", "Manually Adding Error Error Error");
	}

	public void OnAddEventTap(){
    DAS.Event("DebugMenuManager", "Manually Adding Event Event Event");
	}

	public void OnAddDebugTap(){
    DAS.Debug("DebugMenuManager", "Manually Adding Debug Debug Debug");
	}
  #endregion
}
