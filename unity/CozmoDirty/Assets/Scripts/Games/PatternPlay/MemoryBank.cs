using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class MemoryBank {

  public string name;
  public HashSet<BlockPattern> patterns = new HashSet<BlockPattern>();

  public MemoryBank(string name) {
    this.name = name;
  }

  public enum PatternOrientation {
    Horizontal,
    Vertical,
    Mixed
  }

  public override int GetHashCode() {
    // banks are not order specific, so xor all works
    int x = 0;
    foreach (BlockPattern pattern in patterns) {
      x = x ^ pattern.GetHashCode();
    }
    return x;
  }

  public override bool Equals(System.Object obj) {
    if (obj == null) {
      return false;
    }

    MemoryBank p = obj as MemoryBank;
    if ((System.Object)p == null) {
      return false;
    }
    return Equals(p);
  }

  public bool Equals(MemoryBank other) {
    foreach (BlockPattern pattern in patterns) {
      if (!other.Equals(pattern)) {
        return false;
      }
    }
    return true;
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

  public PatternOrientation BankOrientation {
    get {
      PatternOrientation result = PatternOrientation.Horizontal;
      HashSet<BlockPattern>.Enumerator enumer = patterns.GetEnumerator();
      bool nonEmpty = enumer.MoveNext();
      Debug.Assert(nonEmpty);
      if (enumer.Current.verticalStack) {
        result = PatternOrientation.Vertical;
      }
      BlockPattern cur;
      while ((cur = enumer.Current) != null) {
        if (cur.verticalStack && result == PatternOrientation.Horizontal) {
          return PatternOrientation.Mixed;
        }
        else if (!cur.verticalStack && result == PatternOrientation.Vertical) {
          return PatternOrientation.Mixed;
        }
        if (!enumer.MoveNext()) {
          break;
        }
      }
      return result;
    }
  }

  public HashSet<BlockPattern> GetSeenPatterns() {
    return seenPatterns;
  }

  // seen patterns stored in cozmo space that fit the signature
  private HashSet<BlockPattern> seenPatterns = new HashSet<BlockPattern>();

  public int GetNumTotalPatterns()
  {
    return patterns.Count;
  }

  public int GetNumSeenPatterns()
  {
    return seenPatterns.Count;
  }
}
