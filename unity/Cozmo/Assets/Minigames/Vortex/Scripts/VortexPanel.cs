using UnityEngine;
using System.Collections;
using DG.Tweening;

namespace Vortex {

  public class VortexPanel : BaseDialog {

    [SerializeField]
    private UnityEngine.UI.Button _SpinButton;
    [SerializeField]
    private Anki.UI.AnkiTextLabel _StatusText;
    [SerializeField]
    private UnityEngine.UI.Text _ScoreboardText;
    [SerializeField]
    private UnityEngine.UI.Button _DebugTap1Button;
    [SerializeField]
    private UnityEngine.UI.Button _ReplayButton;

    // canvas renderer that we draw the slices on.
    [SerializeField]
    private SpinWheel _SpinWheel;

    public delegate void SpinEventHandler();

    public SpinEventHandler HandleSpinStarted;
    public SpinEventHandler HandleSpinEnded;
    public SpinEventHandler HandleDebugTap;
    public SpinEventHandler HandleReplayClicked;

    void Start() {
      _SpinButton.onClick.AddListener(HandleSpinClick);
      _DebugTap1Button.onClick.AddListener(HandleDebugClick);
      _ReplayButton.onClick.AddListener(HandleReplayClick);
      EnableReplayButton(false);
    }

    public void SetLockSpinner(bool spinnerLocked = false) {
      // This button will be replaced with a drag so this should be unneeded.
      _SpinButton.enabled = !spinnerLocked;
      _SpinButton.gameObject.SetActive(!spinnerLocked);
    }

    public void SetScoreboardText(string str) {
      _ScoreboardText.text = str;
    }

    public void SetStatusText(string str) {
      _StatusText.text = str;
    }

    public int GetWheelNumber() {
      return _SpinWheel.GetNumberOnSelectedSlice();
    }

    private void HandleSpinClick() {
      if (HandleSpinStarted != null) {
        HandleSpinStarted();
      }
      _SpinWheel.StartSpin(HandleSpinComplete);
    }

    private void HandleSpinComplete() {
      if (HandleSpinEnded != null) {
        HandleSpinEnded();
      }
    }

    private void HandleDebugClick() {
      if (HandleDebugTap != null) {
        HandleDebugTap();
      }
    }

    public void EnableReplayButton(bool show) {
      _ReplayButton.gameObject.SetActive(show);
    }

    private void HandleReplayClick() {
      if (HandleReplayClicked != null) {
        HandleReplayClicked();
      }
    }

    protected override void CleanUp() {
    }

    protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
    }

  }

}
