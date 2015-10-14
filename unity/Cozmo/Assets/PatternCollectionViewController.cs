using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternCollectionViewController : MonoBehaviour {
  public PatternDisplay horizontalPatternDisplay;

  public BlockPattern[] testPatterns;
	// Use this for initialization
	void Start () {
    testPatterns = new BlockPattern[3];
    testPatterns[0] = new BlockPattern();
    testPatterns[0].blocks = new List<BlockLights> { 
      new BlockLights{ front = false, back = true, left = false, right = false },
      new BlockLights{ front = false, back = true, left = false, right = false },
      new BlockLights{ front = false, back = true, left = false, right = false },
      new BlockLights{ front = false, back = true, left = false, right = false },
    };

    testPatterns[1] = new BlockPattern();
    testPatterns[1].blocks = new List<BlockLights> { 
      new BlockLights{ front = true, back = false, left = false, right = false },
      new BlockLights{ front = true, back = false, left = false, right = false },
      new BlockLights{ front = true, back = false, left = false, right = false },
      new BlockLights{ front = true, back = false, left = false, right = false },
    };

    testPatterns[2] = new BlockPattern();
    testPatterns[2].blocks = new List<BlockLights> { 
      new BlockLights{ front = false, back = false, left = false, right = false },
      new BlockLights{ front = false, back = false, left = false, right = false },
      new BlockLights{ front = false, back = false, left = false, right = false },
      new BlockLights{ front = false, back = false, left = false, right = false },
    };

    horizontalPatternDisplay.pattern = testPatterns[2];
	}

  public void onOneClicked() {
    horizontalPatternDisplay.pattern = testPatterns[0];
  }

  public void onTwoClicked() {
    horizontalPatternDisplay.pattern = testPatterns[1];
  }

  public void onThreeClicked() {
    horizontalPatternDisplay.pattern = testPatterns[2];
  }
	
	// Update is called once per frame
	void Update () {
	
	}
}
