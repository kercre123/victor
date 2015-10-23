using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;

public class PatternCollectionViewController : MonoBehaviour {

  [SerializeField]
  private BadgeDisplay _openPatternCollectionDialogButtonPrefab;

  private BadgeDisplay _buttonBadgeDisplay;
 
  [SerializeField]
  private PatternCollectionDialog _patternCollectionDialogPrefab;

  private PatternCollectionDialog _patternCollectionDialog;

  [SerializeField]
  private PatternDiscoveredDialog _patternDiscoveredDialogPrefab;
  
  private PatternDiscoveredDialog _patternDiscoveredDialog;

  private float _lastOpenedScrollValue = 0;
  
  [SerializeField]
  private PatternPlayInstructions _patternPlayInstructionsPrefab;

  private PatternMemory _patternMemory;
  
  private void OnDestroy() {
    //_patternPlayInstructions.InstructionsFinished -= OnInstructionsFinished;
    
    if (_patternCollectionDialog != null) {
      _patternCollectionDialog.DialogClosed -= OnCollectionDialogClose;
    }
    
    _patternMemory.PatternAdded -= OnPatternAdded;
  }

  #region Initialization
  public void Initialize(PatternMemory patternMemory) {
    _patternMemory = patternMemory;
    _patternMemory.PatternAdded += OnPatternAdded;

    CreateDialogButton ();
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
    UIManager.CloseAllDialogs ();
    ShowUnlockMomentDialog (pattern);
  }

  private void ShowUnlockMomentDialog(BlockPattern pattern) {
    BaseDialog newDialog = UIManager.OpenDialog (_patternDiscoveredDialogPrefab);
    _patternDiscoveredDialog = newDialog as PatternDiscoveredDialog;
    
    _patternDiscoveredDialog.DialogClosed += OnDiscoveryDialogClosed;
  }

  private void OnDiscoveryDialogClosed() {
    PlayAddPatternToBankAnimation ();
  }

  private void PlayAddPatternToBankAnimation() {
    // TODO: Play animation
    OnAddPatternToBankAnimationFinished ();
  }

  private void OnAddPatternToBankAnimationFinished() {
    // Update badge visuals
    _buttonBadgeDisplay.UpdateDisplayWithTag (PatternMemory.PATTERN_MEMORY_BADGE_TAG);
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

  #region UI Testing Helpers
  public void AddRandomPattern() {
    bool addedPattern = false;
    do {
      bool randFront = FlipCoin ();
      bool randBack = FlipCoin ();
      bool randLeft = FlipCoin ();
      bool randRight = FlipCoin ();
      bool randFacing = FlipCoin ();
      bool randStack = FlipCoin ();

      BlockPattern newPattern = new BlockPattern ();
      newPattern.blocks = new List<BlockLights> {
        new BlockLights{ front = randFront, back = randBack, left = randLeft, right = randRight, facing_cozmo = randFacing },
        new BlockLights{ front = randFront, back = randBack, left = randLeft, right = randRight, facing_cozmo = randFacing },
        new BlockLights{ front = randFront, back = randBack, left = randLeft, right = randRight, facing_cozmo = randFacing }
      };
      newPattern.verticalStack = randStack;
      addedPattern = _patternMemory.AddSeen (newPattern);
    } while (!addedPattern);
  }

  private bool FlipCoin()
  {
    return Random.Range (0f, 1f) > 0.5f;
  }

	public void CreateTestPatternMemory()	{
		PatternMemory patternMemory = new PatternMemory ();
		patternMemory.Initialize ();

		// Add fake seen patterns to memory
    BlockPattern newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = true },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = true },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = true }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);

    BadgeManager.TryRemoveBadge (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true },
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true },
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);
    
    BadgeManager.TryRemoveBadge (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false }
    };
    newPattern.verticalStack = false;
    patternMemory.AddSeen (newPattern);
    BadgeManager.TryRemoveBadge (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true }
    };
    newPattern.verticalStack = false;
    patternMemory.AddSeen (newPattern);
    BadgeManager.TryRemoveBadge (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);
    BadgeManager.TryRemoveBadge (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = true }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = false },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = false },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = false }
    };
    newPattern.verticalStack = false;
    patternMemory.AddSeen (newPattern);

    Initialize (patternMemory);
	}
  #endregion
}
