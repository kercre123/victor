using UnityEngine;
using System.Collections;

public class DebugMenuManager : MonoBehaviour {

	[SerializeField]
	private DebugMenuDialog debugMenuDialogPrefab_;
  private DebugMenuDialog debugMenuDialogInstance_;

  [SerializeField]
  private Canvas debugMenuCanvas_;

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
  }
}
