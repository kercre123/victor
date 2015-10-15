using UnityEngine;
using System.Collections;

public class PatternPlayInstructions : MonoBehaviour {

  public delegate void PatternPlayInstructionsHandler();
  public event PatternPlayInstructionsHandler InstructionsFinished;
  private void RaiseInstructionsFinished() {
    if (InstructionsFinished != null)
      InstructionsFinished ();
  }

  // TODO: Make this persist
  private int _currentInstructionIndex = 0;

  [SerializeField]
  private GameObjectSelector _instructionDisplays;

  public void Initialize()
  {
    _instructionDisplays.SetScreenIndex (_currentInstructionIndex);
  }

  public void OnNextButtonClick()
  {
    ContinueToNextInstruction ();
  }

  public void ContinueToNextInstruction()
  {
    _currentInstructionIndex++;
    if (_currentInstructionIndex >= _instructionDisplays.GetNumGameObjects ()) {
      RaiseInstructionsFinished ();
    } else {
      _instructionDisplays.SetScreenIndex (_currentInstructionIndex);
    }
  }
}
