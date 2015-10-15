using UnityEngine;
using System.Collections;
using System.Collections.Generic;

// Temp class for testing dialog functionality. Should have PatternCollectionDialog
// hook up to PatternPlayUIController later
public class PatternCollectionViewController : MonoBehaviour {
 
  [SerializeField]
  private PatternCollectionDialog _patternCollectionDialog;

  private PatternMemory _testPatternMemory;
  
  // Use this for initialization
  void Start () {
  	// Set up a test PatternMemory object to mimic data from PatternPlayUIController
    _testPatternMemory = CreateTestPatternMemory ();

  	// Populate dialog with cards using memory
    _patternCollectionDialog.Initialize (_testPatternMemory);
  }

	private PatternMemory CreateTestPatternMemory()
	{
		PatternMemory patternMemory = new PatternMemory ();
		patternMemory.Initialize ();

		// TODO: Add seen patterns to memory

		return patternMemory;
	}
}
