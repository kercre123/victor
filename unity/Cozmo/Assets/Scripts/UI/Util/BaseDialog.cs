using UnityEngine;
using System.Collections;

public class BaseDialog : MonoBehaviour {
  
  public delegate void SimpleBaseDialogHandler();
  public delegate void BaseDialogHandler(string dialogId, BaseDialog dialog);

  public event SimpleBaseDialogHandler DialogOpened;
  public static event BaseDialogHandler BaseDialogOpened;
  public static void RaiseDialogOpened(BaseDialog dialog) {
    if (dialog.DialogOpened != null) {
      dialog.DialogOpened ();
    }
    if (BaseDialogOpened != null) {
      BaseDialogOpened (dialog.Id, dialog);
    }
  }

  public event SimpleBaseDialogHandler DialogClosed;
  public static event BaseDialogHandler BaseDialogClosed;
  public static void RaiseDialogClosed(BaseDialog dialog) {
    if (dialog.DialogClosed != null) {
      dialog.DialogClosed ();
    }
    if (BaseDialogClosed != null) {
      BaseDialogClosed(dialog.Id, dialog);
    }
  }

  [SerializeField]
  private string _dialogId = "Dialog";
  public string Id {
    get { return _dialogId; }
    private set { _dialogId = value; }
  }

  public void OpenDialog() {
    RaiseDialogOpened (this);
    PlayOpenAnimations ();
  }

  public void CloseDialog() {
    RaiseDialogClosed (this);
    PlayCloseAnimations ();
  }

  private void PlayOpenAnimations() {
    // TODO: Play some animations
    OnOpenAnimationsFinished ();
  }

  private void OnOpenAnimationsFinished() {
    // TODO: Raise event
  }

  private void PlayCloseAnimations() {
    // TODO: Play some animations
    OnCloseAnimationsFinished ();
  }

  private void OnCloseAnimationsFinished() {
    // TODO: Raise event
    GameObject.Destroy (gameObject);
  }
}
