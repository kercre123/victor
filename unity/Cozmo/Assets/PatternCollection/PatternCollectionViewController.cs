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

		// Add fake seen patterns to memory
    BlockPattern newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = false, right = false },
      new BlockLights{ front = false, back = true, left = false, right = false },
      new BlockLights{ front = false, back = true, left = false, right = false }
    };
    newPattern.facingCozmo = true;
    newPattern.verticalStack = true;
    patternMemory.Add (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = false, left = false, right = true },
      new BlockLights{ front = false, back = false, left = false, right = true },
      new BlockLights{ front = false, back = false, left = false, right = true }
    };
    newPattern.facingCozmo = true;
    newPattern.verticalStack = true;
    patternMemory.Add (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = true, right = false },
      new BlockLights{ front = false, back = true, left = true, right = false },
      new BlockLights{ front = false, back = true, left = true, right = false }
    };
    newPattern.facingCozmo = false;
    newPattern.verticalStack = false;
    patternMemory.Add (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = false },
      new BlockLights{ front = true, back = true, left = true, right = false },
      new BlockLights{ front = true, back = true, left = true, right = false }
    };
    newPattern.facingCozmo = true;
    newPattern.verticalStack = false;
    patternMemory.Add (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = false, right = false },
      new BlockLights{ front = false, back = true, left = false, right = false },
      new BlockLights{ front = false, back = true, left = false, right = false }
    };
    newPattern.facingCozmo = false;
    newPattern.verticalStack = true;
    patternMemory.Add (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = true },
      new BlockLights{ front = true, back = true, left = true, right = true },
      new BlockLights{ front = true, back = true, left = true, right = true }
    };
    newPattern.facingCozmo = true;
    newPattern.verticalStack = true;
    patternMemory.Add (newPattern);

    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = true },
      new BlockLights{ front = true, back = true, left = true, right = true },
      new BlockLights{ front = true, back = true, left = true, right = true }
    };
    newPattern.facingCozmo = false;
    newPattern.verticalStack = false;
    patternMemory.Add (newPattern);

		return patternMemory;
	}
}
