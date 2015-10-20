using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;

// Temp class for testing dialog functionality. Should have PatternCollectionDialog
// hook up to PatternPlayUIController later
public class PatternCollectionViewController : MonoBehaviour {

  [SerializeField]
  private Button _openPatternCollectionDialogButtonPrefab;
 
  [SerializeField]
  private PatternCollectionDialog _patternCollectionDialogPrefab;

  private PatternCollectionDialog _patternCollectionDialog;

  private float _lastOpenedScrollValue = 0;
  
  [SerializeField]
  private PatternPlayInstructions _patternPlayInstructionsPrefab;

  private PatternMemory _testPatternMemory;
  
  // Use this for initialization
  private void Start () {
  	// Set up a test PatternMemory object to mimic data from PatternPlayUIController
    _testPatternMemory = CreateTestPatternMemory ();

    CreateDialogButton ();
  }

  private void OnDestroy()
  {
    //_patternPlayInstructions.InstructionsFinished -= OnInstructionsFinished;

    if (_patternCollectionDialog != null) {
      _patternCollectionDialog.DialogClosed -= OnCollectionDialogClose;
    }
  }

  private void CreateDialogButton()
  {
    GameObject newButton = UIManager.CreateUI (_openPatternCollectionDialogButtonPrefab.gameObject);
    Button buttonScript = newButton.GetComponent<Button> ();
    buttonScript.enabled = false;
    buttonScript.enabled = true;
    buttonScript.onClick.AddListener (OnDialogButtonTap);
  }

  public void OnDialogButtonTap()
  {
    ShowCollectionDialog ();
  }

  private void ShowCollectionDialog()
  {
    BaseDialog newDialog = UIManager.OpenDialog (_patternCollectionDialogPrefab);
    _patternCollectionDialog = newDialog as PatternCollectionDialog;

    // Populate dialog with cards using memory
    _patternCollectionDialog.Initialize (_testPatternMemory);
    _patternCollectionDialog.SetScrollValue (_lastOpenedScrollValue);
    _patternCollectionDialog.DialogClosed += OnCollectionDialogClose;
  }

  private void OnCollectionDialogClose()
  {
    _patternCollectionDialog.DialogClosed -= OnCollectionDialogClose;
    _lastOpenedScrollValue = _patternCollectionDialog.GetScrollValue ();
  }

  private void ShowInstructionsDialog()
  {
    // Show instructions
    // TODO: Instantiate instructions
    // _patternPlayInstructions.gameObject.SetActive (true);
    // _patternPlayInstructions.Initialize ();
    // _patternPlayInstructions.InstructionsFinished += OnInstructionsFinished;
  }
  
  private void OnInstructionsFinished()
  {
    // Destroy instructions dialog
    // Create button?
  }

	private PatternMemory CreateTestPatternMemory()
	{
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

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true },
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true },
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false }
    };
    newPattern.verticalStack = false;
    patternMemory.AddSeen (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true }
    };
    newPattern.verticalStack = false;
    patternMemory.AddSeen (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);

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

		return patternMemory;
	}
}
