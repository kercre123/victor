using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using DG.Tweening;

public class UIManager : MonoBehaviour {

  private static readonly IDAS sDAS = DAS.GetInstance(typeof(UIManager));

  private static UIManager _Instance;

  public static UIManager Instance {
    get {
      if (_Instance == null) {
        sDAS.Error("Don't access this until Start!");
      }
      return _Instance;
    }
    private set {
      if (_Instance != null) {
        sDAS.Error("There shouldn't be more than one UIManager");
      }
      _Instance = value;
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
    _Instance = this;
    _openDialogs = new List<BaseDialog>();
    DOTween.Init();
    BaseDialog.BaseDialogCloseAnimationFinished += HandleBaseDialogCloseAnimationFinished;
  }

  public static GameObject CreateUI(GameObject uiPrefab) {
    return CreateUI(uiPrefab, Instance._orthoUiCanvas.transform);
  }

  public static GameObject CreatePerspectiveUI(GameObject uiPrefab) {
    GameObject newUi = GameObject.Instantiate(uiPrefab);
    newUi.transform.SetParent(Instance._perspUiCanvas.transform, false);
    newUi.layer = LayerMask.NameToLayer("3DUI");
    return newUi;
  }

  public static GameObject CreateUI(GameObject uiPrefab, Transform parentTransform) {
    GameObject newUi = GameObject.Instantiate(uiPrefab);
    newUi.transform.SetParent(parentTransform, false);
    return newUi;
  }

  public static BaseDialog OpenDialog(BaseDialog dialogPrefab) {
    GameObject newDialog = GameObject.Instantiate(dialogPrefab.gameObject);
    newDialog.transform.SetParent(Instance._orthoUiCanvas.transform, false);

    BaseDialog baseDialogScript = newDialog.GetComponent<BaseDialog>();
    baseDialogScript.OpenDialog();
    Instance._openDialogs.Add(baseDialogScript);

    return baseDialogScript;
  }

  public static void CloseDialog(BaseDialog dialogObject) {
    dialogObject.CloseDialog();
  }

  public static void CloseDialogImmediately(BaseDialog dialogObject) {
    dialogObject.CloseDialogImmediately();
  }

  public static void CloseAllDialogs() {
    while (Instance._openDialogs.Count > 0) {
      if (Instance._openDialogs[0] != null) {
        Instance._openDialogs[0].CloseDialog();
      }
    }
  }

  public static void CloseAllDialogsImmediately() {
    while (Instance._openDialogs.Count > 0) {
      if (Instance._openDialogs[0] != null) {
        Instance._openDialogs[0].CloseDialogImmediately();
      }
    }
  }

  public static Camera GetUICamera() {
    return _Instance._orthoUiCanvas.worldCamera;
  }

  public static void DisableTouchEvents() {
    _Instance._eventSystemScript.gameObject.SetActive(false);
  }

  public static void EnableTouchEvents() {
    _Instance._eventSystemScript.gameObject.SetActive(true);
  }

  private void HandleBaseDialogCloseAnimationFinished(string dialogId, BaseDialog dialog) {
    Instance._openDialogs.Remove(dialog);
  }

}
