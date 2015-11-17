using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using DG.Tweening;

namespace PatternPlay {

  public class PatternCollectionViewController : MonoBehaviour {

    [SerializeField]
    private BadgeDisplay _OpenPatternCollectionDialogButtonPrefab;
    private BadgeDisplay _ButtonBadgeDisplay;
    private Tweener _ButtonBadgeDisplayTweener;

    [SerializeField]
    private PatternCollectionDialog _PatternCollectionDialogPrefab;

    private PatternCollectionDialog _PatternCollectionDialog;

    [SerializeField]
    private PatternDiscoveredDialog _PatternDiscoveredDialogPrefab;

    private PatternDiscoveredDialog _PatternDiscoveredDialog;

    private float _LastOpenedScrollValue = 0;

    private PatternMemory _PatternMemory = null;

    // TODO: Pragma out for production
    private void Start() {
      // Listen for debug menu events
      PatternPlayPane.PatternPlayPaneOpened += OnPatternPlayPaneOpened;
    }

    private void OnDestroy() {
      //_patternPlayInstructions.InstructionsFinished -= OnInstructionsFinished;

      if (_PatternCollectionDialog != null) {
        _PatternCollectionDialog.DialogClosed -= OnCollectionDialogClose;
        UIManager.CloseDialogImmediately(_PatternCollectionDialog);
      }

      if (_PatternMemory != null) {
        _PatternMemory.PatternAdded -= OnPatternAdded;
      }

      if (_ButtonBadgeDisplayTweener != null) {
        _ButtonBadgeDisplayTweener.Kill();
      }
      if (_ButtonBadgeDisplay != null) {
        Destroy(_ButtonBadgeDisplay.gameObject);
      }

      if (_PatternDiscoveredDialog != null) {
        _PatternDiscoveredDialog.DialogCloseAnimationFinished -= OnDiscoveryDialogClosed;
        UIManager.CloseDialogImmediately(_PatternDiscoveredDialog);
      }
    }

    #region Initialization

    public void Initialize(PatternMemory patternMemory) {
      if (_PatternMemory == null) {
        _PatternMemory = patternMemory;
        _PatternMemory.PatternAdded += OnPatternAdded;

        CreateDialogButton();
      }
      else {
        DAS.Error("PatternCollectionViewController", 
          "Tried to Initialize with a new PatternMemory when one already exists!");
      }
    }

    #endregion

    #region Memory Bank Dialog

    private void CreateDialogButton() {
      GameObject newButton = UIManager.CreateUI(_OpenPatternCollectionDialogButtonPrefab.gameObject);

      Button buttonScript = newButton.GetComponent<Button>();
      buttonScript.enabled = false;
      buttonScript.enabled = true;
      buttonScript.onClick.AddListener(OnDialogButtonTap);

      _ButtonBadgeDisplay = newButton.GetComponent<BadgeDisplay>();
      _ButtonBadgeDisplay.UpdateDisplayWithTag(PatternMemory.kPatternMemoryBadgeTag);
    }

    public void OnDialogButtonTap() {
      ShowCollectionDialog();
    }

    private void ShowCollectionDialog() {
      BaseDialog newDialog = UIManager.OpenDialog(_PatternCollectionDialogPrefab);
      _PatternCollectionDialog = newDialog as PatternCollectionDialog;

      // Populate dialog with cards using memory
      _PatternCollectionDialog.Initialize(_PatternMemory);

      if (BadgeManager.NumBadgesWithTag(PatternMemory.kPatternMemoryBadgeTag) > 0) {
        bool scrollSuccess = _PatternCollectionDialog.ScrollToFirstNewPattern();
        if (!scrollSuccess) {
          _PatternCollectionDialog.SetScrollValue(_LastOpenedScrollValue);
        }
      }
      else {
        _PatternCollectionDialog.SetScrollValue(_LastOpenedScrollValue);
      }

      _PatternCollectionDialog.DialogClosed += OnCollectionDialogClose;
    }

    private void OnCollectionDialogClose() {
      _ButtonBadgeDisplay.UpdateDisplayWithTag(PatternMemory.kPatternMemoryBadgeTag);
      _PatternCollectionDialog.DialogClosed -= OnCollectionDialogClose;
      _LastOpenedScrollValue = _PatternCollectionDialog.GetScrollValue();
    }

    #endregion

    #region Pattern Unlock Moment

    private void OnPatternAdded(BlockPattern pattern, MemoryBank bank) {
      UIManager.CloseAllDialogsImmediately();
      ShowUnlockMomentDialog(pattern);
    }

    private void ShowUnlockMomentDialog(BlockPattern pattern) {
      BaseDialog newDialog = UIManager.OpenDialog(_PatternDiscoveredDialogPrefab);
      _PatternDiscoveredDialog = newDialog as PatternDiscoveredDialog;
      _PatternDiscoveredDialog.Initialize(pattern);

      _PatternDiscoveredDialog.DialogCloseAnimationFinished += OnDiscoveryDialogClosed;
    }

    private void OnDiscoveryDialogClosed() {
      // Update badge visuals
      _ButtonBadgeDisplay.UpdateDisplayWithTag(PatternMemory.kPatternMemoryBadgeTag);

      float targetScale = 0.2f;
      if (_ButtonBadgeDisplayTweener == null) {
        _ButtonBadgeDisplayTweener = _ButtonBadgeDisplay.gameObject.transform.DOPunchScale(new Vector3(targetScale, targetScale, targetScale),
          0.5f,
          15)
          .SetAutoKill(false); // Let the tween stick around in memory
      }
      _ButtonBadgeDisplayTweener.Restart();
      _ButtonBadgeDisplayTweener.Play();
    }

    #endregion

    #region Instructions dialog

    private void ShowInstructionsDialog() {
      // Show instructions
      // TODO: Instantiate instructions
      // _patternPlayInstructions.gameObject.SetActive (true);
      // _patternPlayInstructions.Initialize ();
      // _patternPlayInstructions.InstructionsFinished += OnInstructionsFinished;
    }

    private void OnInstructionsFinished() {
      // Destroy instructions dialog
      // Create button?
    }

    #endregion

    // TODO pragma out for production

    #region DebugMenu

    private void OnPatternPlayPaneOpened(PatternPlayPane patternPlayPane) {
      patternPlayPane.Initialize(this);
    }

    public bool IsInitialized() {
      return _PatternMemory != null;
    }

    public bool TryAddPattern(BlockPattern pattern) {
      bool success = false;
      if (IsInitialized()) {
        success = _PatternMemory.AddSeen(pattern);
      }
      return success;
    }

    public void AddAllUnseenPatterns() {
      if (IsInitialized()) {
        HashSet<BlockPattern> unseenPatterns = _PatternMemory.GetAllUnseenPatterns();
        foreach (BlockPattern pattern in unseenPatterns) {
          _PatternMemory.AddSeen(pattern);
        }
      }
    }

    public bool AnyUnseenPatterns() {
      HashSet<BlockPattern> unseenPatterns = _PatternMemory.GetAllUnseenPatterns();
      return unseenPatterns != null && unseenPatterns.Count > 0;
    }

    #endregion
  }

}
