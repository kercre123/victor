using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using DG.Tweening;

public class UIManager : MonoBehaviour {

  private static UIManager _instance;
  public static UIManager Instance {
    get {
      if (_instance == null) {
        DAS.Error("UIManager", "Don't access this until Start!");
      }
      return _instance;
    }
    private set {
      if (_instance != null) {
        DAS.Error("UIManager", "There shouldn't be more than one UIManager");
      }
      _instance = value;
    }
  }

  [SerializeField]
  private Canvas _orthoUiCanvas;
  
  [SerializeField]
  private Canvas _perspUiCanvas;
  
  [SerializeField]
  private EventSystem _eventSystemScript;

  private List<BaseDialog> _openDialogs;

  void Awake() {
    _instance = this;
    _openDialogs = new List<BaseDialog>();
    DOTween.Init ();
  }

  public static GameObject CreateUI(GameObject uiPrefab) {
    return CreateUI(uiPrefab, Instance._orthoUiCanvas.transform);
  }

  public static GameObject CreatePerspectiveUI(GameObject uiPrefab) {
    GameObject newUi = GameObject.Instantiate (uiPrefab);
    newUi.transform.SetParent (Instance._perspUiCanvas.transform, false);
    newUi.layer = LayerMask.NameToLayer ("3DUI");
    return newUi;
  }

  public static GameObject CreateUI(GameObject uiPrefab, Transform parentTransform) {
    GameObject newUi = GameObject.Instantiate (uiPrefab);
    newUi.transform.SetParent (parentTransform, false);
    return newUi;
  }

  public static BaseDialog OpenDialog(BaseDialog dialogPrefab) {
    GameObject newDialog = GameObject.Instantiate (dialogPrefab.gameObject);
    newDialog.transform.SetParent (Instance._orthoUiCanvas.transform, false);

    BaseDialog baseDialogScript = newDialog.GetComponent<BaseDialog> ();
    baseDialogScript.OpenDialog ();
    Instance._openDialogs.Add (baseDialogScript);

    return baseDialogScript;
  }

  public static void CloseDialog(BaseDialog dialogObject) {
    Instance._openDialogs.Remove (dialogObject);
    dialogObject.CloseDialog ();
  }

  public static void CloseAllDialogs() {
    foreach (BaseDialog dialog in Instance._openDialogs) {
      dialog.CloseDialog();
    }
    Instance._openDialogs.Clear ();
  }

  public static Camera GetUICamera() {
    return _instance._orthoUiCanvas.worldCamera;
  }

  public static void DisableTouchEvents() {
    _instance._eventSystemScript.gameObject.SetActive (false);
  }

  public static void EnableTouchEvents() {
    _instance._eventSystemScript.gameObject.SetActive (true);
  }
}
