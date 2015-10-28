using UnityEngine;
using System.Collections;

public class PatternDiscoveryDisplay : MonoBehaviour {

  [SerializeField]
  private PatternDisplay _stackPatternDisplay;
  
  [SerializeField]
  private PatternDisplay _horizontalPatternDisplay;
  
  public void Initialize(BlockPattern discoveredPattern) {
    if (discoveredPattern.verticalStack) {
      _stackPatternDisplay.pattern = discoveredPattern;
      _horizontalPatternDisplay.pattern = null;
    } else {
      _stackPatternDisplay.pattern = null;
      _horizontalPatternDisplay.pattern = discoveredPattern;
    }
  }
}
