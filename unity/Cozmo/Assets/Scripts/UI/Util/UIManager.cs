using UnityEngine;
using UnityEngine.UI;
using System.Collections;

// In the future we can use this to drive opening and closing dialogs.
public class UIManager : MonoBehaviour {

  private static UIManager _instance;
  public static UIManager Instance
  {
    get {
      if (_instance == null)
      {
        Debug.LogError("Don't access this until Start!");
      }
      return _instance;
    }
    private set {
      if (_instance != null)
      {
        Debug.LogError("There shouldn't be more than one UIManager");
      }
      _instance = value;
    }
  }

  [SerializeField]
  private Canvas _sceneCanvas;

  void Awake()
  {
    _instance = this;
  }

  public static GameObject CreateUI(GameObject uiPrefab)
  {
    GameObject newUi = GameObject.Instantiate (uiPrefab);
    newUi.transform.SetParent (Instance._sceneCanvas.transform, false);
    return newUi;
  }

  public static GameObject CreateUI(GameObject uiPrefab, Transform parentTransform)
  {
    GameObject newUi = GameObject.Instantiate (uiPrefab);
    newUi.transform.SetParent (parentTransform, false);
    return newUi;
  }

  public static BaseDialog OpenDialog(BaseDialog dialogPrefab)
  {
    GameObject newDialog = GameObject.Instantiate (dialogPrefab.gameObject);
    newDialog.transform.SetParent (Instance._sceneCanvas.transform, false);

    BaseDialog baseDialogScript = newDialog.GetComponent<BaseDialog> ();
    baseDialogScript.OpenDialog ();

    return baseDialogScript;
  }

  public static void CloseDialog(BaseDialog dialogObject)
  {
    dialogObject.CloseDialog ();
  }
}
