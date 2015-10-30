using UnityEngine;
using System.Collections;

public class DebugMenuManager : MonoBehaviour {

	[SerializeField]
	private DebugMenuDialog debugMenuDialogPrefab_;
  private DebugMenuDialog debugMenuDialogInstance_;

  void Start(){
    // TODO: Pragma out this code for production
    GameObject debugMenuDialogInstance = GameObject.Instantiate(debugMenuDialogPrefab_.gameObject);
    Transform dialogTransform = debugMenuDialogInstance.transform;
    dialogTransform.SetParent(this.transform);
    dialogTransform.localPosition = Vector3.zero;
    debugMenuDialogInstance_ = debugMenuDialogInstance.GetComponent<DebugMenuDialog>();
  }
}
