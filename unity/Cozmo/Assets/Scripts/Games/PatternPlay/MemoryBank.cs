using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class MemoryBank {

  public string name;
  public HashSet<BlockPattern> patterns = new HashSet<BlockPattern>();
  public bool verticalStack = false;
  public bool facingCozmo = false;

  public MemoryBank(string name) {
    this.name = name;
  }

  public bool Add(BlockPattern newPattern) {
    return patterns.Add(newPattern);
  }
  public bool Contains(BlockPattern newPattern) {
    return patterns.Contains(newPattern);
  }

  public bool TryAddSeen(BlockPattern newPattern) {
    if (patterns.Contains(newPattern)) {
      return seenPatterns.Add(newPattern);
    }
    return false;
  }

  public HashSet<BlockPattern> GetSeenPatterns() {
    return seenPatterns;
  }

  public bool IsVertical { get { return verticalStack; } }

  // seen patterns stored in cozmo space that fit the signature
  private HashSet<BlockPattern> seenPatterns = new HashSet<BlockPattern>();

}
