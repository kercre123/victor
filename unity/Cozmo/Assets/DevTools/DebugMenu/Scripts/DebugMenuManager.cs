using UnityEngine;
using System.Collections;

public class DebugMenuManager : MonoBehaviour {

  [SerializeField]
  private DebugMenuDialog _DebugMenuDialogPrefab;
  private DebugMenuDialog _DebugMenuDialogInstance;

  [SerializeField]
  private Canvas _DebugMenuCanvas;

  private int _LastOpenedDebugTab = 0;

  void Start() {
    // TODO: Destroy self if not production
  }
  
  // TODO: Pragma out this code for production
  public void OnDebugMenuButtonTap() {
    CreateDebugDialog();
  }

  private void CreateDebugDialog() {
    GameObject debugMenuDialogInstance = GameObject.Instantiate(_DebugMenuDialogPrefab.gameObject);
    Transform dialogTransform = debugMenuDialogInstance.transform;
    dialogTransform.SetParent(_DebugMenuCanvas.transform, false);
    dialogTransform.localPosition = Vector3.zero;
    _DebugMenuDialogInstance = debugMenuDialogInstance.GetComponent<DebugMenuDialog>();
    _DebugMenuDialogInstance.Initialize(_LastOpenedDebugTab);
    _DebugMenuDialogInstance.OnDebugMenuClosed += OnDebugMenuDialogClose;
  }

  private void OnDebugMenuDialogClose(int lastOpenTab) {
    _DebugMenuDialogInstance.OnDebugMenuClosed -= OnDebugMenuDialogClose;
    _LastOpenedDebugTab = lastOpenTab;
  }
}
