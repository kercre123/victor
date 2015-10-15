using UnityEngine;
using System.Collections;

public class PatternCollectionBankCard : MonoBehaviour {

  public void Initialize(MemoryBank memoryBank) {
    // TODO: Remove magic numbers! Assume 4 matches per memory bank
    Debug.LogError ("Initializing bank card with bank: " + memoryBank.Signature.ToString ());
  }
  
  private void ShowSeenPatterns() {
  }
  
  private void ShowUnseenPatterns() {
  }
}
