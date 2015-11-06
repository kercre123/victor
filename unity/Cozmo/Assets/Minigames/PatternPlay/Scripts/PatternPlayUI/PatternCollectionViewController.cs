using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using DG.Tweening;

public class PatternCollectionViewController : MonoBehaviour {

  [SerializeField]
  private BadgeDisplay _openPatternCollectionDialogButtonPrefab;
  private BadgeDisplay _buttonBadgeDisplay;
  private Tweener _buttonBadgeDisplayTweener;
 
  [SerializeField]
  private PatternCollectionDialog _patternCollectionDialogPrefab;

  private PatternCollectionDialog _patternCollectionDialog;

  [SerializeField]
  private PatternDiscoveredDialog _patternDiscoveredDialogPrefab;
  
  private PatternDiscoveredDialog _patternDiscoveredDialog;

  private float _lastOpenedScrollValue = 0;

  private PatternMemory _patternMemory = null;

  // TODO: Pragma out for production
  private void Start(){
    // Listen for debug menu events
    PatternPlayPane.PatternPlayPaneOpened += OnPatternPlayPaneOpened;
  }
  
  private void OnDestroy() {
    //_patternPlayInstructions.InstructionsFinished -= OnInstructionsFinished;
    
    if (_patternCollectionDialog != null) {
      _patternCollectionDialog.DialogClosed -= OnCollectionDialogClose;
      UIManager.CloseDialogImmediately(_patternCollectionDialog);
    }

    if (_patternMemory != null) {
      _patternMemory.PatternAdded -= OnPatternAdded;
    }

    if (_buttonBadgeDisplayTweener != null) {
      _buttonBadgeDisplayTweener.Kill();
    }
    if (_buttonBadgeDisplay != null) {
      Destroy(_buttonBadgeDisplay.gameObject);
    }

    if (_patternDiscoveredDialog != null) {
      _patternDiscoveredDialog.DialogCloseAnimationFinished -= OnDiscoveryDialogClosed;
      UIManager.CloseDialogImmediately(_patternDiscoveredDialog);
    }
  }

  #region Initialization
  public void Initialize(PatternMemory patternMemory) {
    if (_patternMemory == null) {
      _patternMemory = patternMemory;
      _patternMemory.PatternAdded += OnPatternAdded;

      CreateDialogButton();
    } else {
      DAS.Error("PatternCollectionViewController", 
                "Tried to Initialize with a new PatternMemory when one already exists!");
    }
  }
  #endregion

  #region Memory Bank Dialog
  private void CreateDialogButton() {
    GameObject newButton = UIManager.CreateUI (_openPatternCollectionDialogButtonPrefab.gameObject);

    Button buttonScript = newButton.GetComponent<Button> ();
    buttonScript.enabled = false;
    buttonScript.enabled = true;
    buttonScript.onClick.AddListener (OnDialogButtonTap);

    _buttonBadgeDisplay = newButton.GetComponent<BadgeDisplay> ();
    _buttonBadgeDisplay.UpdateDisplayWithTag (PatternMemory.PATTERN_MEMORY_BADGE_TAG);
  }

  public void OnDialogButtonTap() {
    ShowCollectionDialog ();
  }

  private void ShowCollectionDialog() {
    BaseDialog newDialog = UIManager.OpenDialog (_patternCollectionDialogPrefab);
    _patternCollectionDialog = newDialog as PatternCollectionDialog;

    // Populate dialog with cards using memory
    _patternCollectionDialog.Initialize (_patternMemory);

    if (BadgeManager.NumBadgesWithTag (PatternMemory.PATTERN_MEMORY_BADGE_TAG) > 0) {
      bool scrollSuccess = _patternCollectionDialog.ScrollToFirstNewPattern();
      if (!scrollSuccess) {
        _patternCollectionDialog.SetScrollValue(_lastOpenedScrollValue);
      }
    } else {
      _patternCollectionDialog.SetScrollValue (_lastOpenedScrollValue);
    }

    _patternCollectionDialog.DialogClosed += OnCollectionDialogClose;
  }

  private void OnCollectionDialogClose() {
    _buttonBadgeDisplay.UpdateDisplayWithTag (PatternMemory.PATTERN_MEMORY_BADGE_TAG);
    _patternCollectionDialog.DialogClosed -= OnCollectionDialogClose;
    _lastOpenedScrollValue = _patternCollectionDialog.GetScrollValue ();
  }
  #endregion

  #region Pattern Unlock Moment
  private void OnPatternAdded(BlockPattern pattern, MemoryBank bank) {
    UIManager.CloseAllDialogsImmediately ();
    ShowUnlockMomentDialog (pattern);
  }

  private void ShowUnlockMomentDialog(BlockPattern pattern) {
    BaseDialog newDialog = UIManager.OpenDialog (_patternDiscoveredDialogPrefab);
    _patternDiscoveredDialog = newDialog as PatternDiscoveredDialog;
    _patternDiscoveredDialog.Initialize (pattern);
    
    _patternDiscoveredDialog.DialogCloseAnimationFinished += OnDiscoveryDialogClosed;
  }

  private void OnDiscoveryDialogClosed() {
    // Update badge visuals
    _buttonBadgeDisplay.UpdateDisplayWithTag (PatternMemory.PATTERN_MEMORY_BADGE_TAG);

    float targetScale = 0.2f;
    if (_buttonBadgeDisplayTweener == null) {
      _buttonBadgeDisplayTweener = _buttonBadgeDisplay.gameObject.transform.DOPunchScale (new Vector3 (targetScale, targetScale, targetScale),
                                                                             0.5f,
                                                                             15)
        .SetAutoKill(false); // Let the tween stick around in memory
    }
    _buttonBadgeDisplayTweener.Restart ();
    _buttonBadgeDisplayTweener.Play ();
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
  private void OnPatternPlayPaneOpened (PatternPlayPane patternPlayPane)
  {
    patternPlayPane.Initialize(this);
  }

  public bool IsInitialized(){
    return _patternMemory != null;
  }

  public bool TryAddPattern(BlockPattern pattern){
    bool success = false;
    if (IsInitialized())  {
      success = _patternMemory.AddSeen(pattern);
    }
    return success;
  }

  public void AddAllUnseenPatterns(){
    if (IsInitialized()) {
      HashSet<BlockPattern> unseenPatterns = _patternMemory.GetAllUnseenPatterns();
      foreach (BlockPattern pattern in unseenPatterns) {
        _patternMemory.AddSeen(pattern);
      }
    }
  }

  public bool AnyUnseenPatterns(){
    HashSet<BlockPattern> unseenPatterns = _patternMemory.GetAllUnseenPatterns();
    return unseenPatterns != null && unseenPatterns.Count > 0;
  }
  #endregion
}
