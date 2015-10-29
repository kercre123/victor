using UnityEngine;
using System.Collections;
using DG.Tweening;

public abstract class BaseDialog : MonoBehaviour {
  
  public delegate void SimpleBaseDialogHandler();
  public delegate void BaseDialogHandler(string dialogId, BaseDialog dialog);

  public event SimpleBaseDialogHandler DialogOpened;
  public static event BaseDialogHandler BaseDialogOpened;
  private static void RaiseDialogOpened(BaseDialog dialog) {
    if (dialog.DialogOpened != null) {
      dialog.DialogOpened ();
    }
    if (BaseDialogOpened != null) {
      BaseDialogOpened (dialog.Id, dialog);
    }
  }

  public event SimpleBaseDialogHandler DialogClosed;
  public static event BaseDialogHandler BaseDialogClosed;
  private static void RaiseDialogClosed(BaseDialog dialog) {
    if (dialog.DialogClosed != null) {
      dialog.DialogClosed ();
    }
    if (BaseDialogClosed != null) {
      BaseDialogClosed(dialog.Id, dialog);
    }
  }

  public event SimpleBaseDialogHandler DialogCloseAnimationFinished;
  public static event BaseDialogHandler BaseDialogCloseAnimationFinished;
  private static void RaiseDialogCloseAnimationFinished(BaseDialog dialog) {
    if (dialog.DialogCloseAnimationFinished != null) {
      dialog.DialogCloseAnimationFinished ();
    }
    if (BaseDialogCloseAnimationFinished != null) {
      BaseDialogCloseAnimationFinished(dialog.Id, dialog);
    }
  }

  [SerializeField]
  private string _dialogId = "Dialog";
  public string Id {
    get { return _dialogId; }
    private set { _dialogId = value; }
  }

  private Sequence _closeAnimation;

  public void OnDestroy() {
    if (_closeAnimation != null) {
      _closeAnimation.Kill();
    }

    CleanUp ();
  }

  protected abstract void CleanUp ();

  public void OpenDialog() {
    RaiseDialogOpened (this);
    PlayOpenAnimations ();
  }

  public void CloseDialog() {
    RaiseDialogClosed (this);
    PlayCloseAnimations ();
  }

  private void PlayOpenAnimations() {
    UIManager.DisableTouchEvents ();

    // TODO: Play some animations

    OnOpenAnimationsFinished ();
  }

  private void OnOpenAnimationsFinished() {
    UIManager.EnableTouchEvents ();

    // TODO: Raise event
  }

  private void PlayCloseAnimations() {
    UIManager.DisableTouchEvents ();

    // Play some animations
    _closeAnimation = DOTween.Sequence ();
    ConstructCloseAnimation (_closeAnimation);
    _closeAnimation.AppendCallback(OnCloseAnimationsFinished);

  }

  protected abstract void ConstructCloseAnimation (Sequence closeAnimation);

  private void OnCloseAnimationsFinished() {
    UIManager.EnableTouchEvents ();
    RaiseDialogCloseAnimationFinished (this);

    GameObject.Destroy (gameObject);
  }
}
