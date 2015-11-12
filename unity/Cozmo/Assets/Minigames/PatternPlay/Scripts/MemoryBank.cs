using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace PatternPlay {

  public class MemoryBank {

    public string Name;
    public HashSet<BlockPattern> Patterns = new HashSet<BlockPattern>();

    public MemoryBank(string name) {
      this.Name = name;
    }

    public enum PatternOrientation {
      Horizontal,
      Vertical,
      Mixed
    }

    public override int GetHashCode() {
      // banks are not order specific, so xor all works
      int x = 0;
      foreach (BlockPattern pattern in Patterns) {
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
      foreach (BlockPattern pattern in Patterns) {
        if (!other.Equals(pattern)) {
          return false;
        }
      }
      return true;
    }

    public bool Add(BlockPattern newPattern) {
      return Patterns.Add(newPattern);
    }

    public bool Contains(BlockPattern newPattern) {
      return Patterns.Contains(newPattern);
    }

    public bool TryAddSeen(BlockPattern newPattern) {
      if (Patterns.Contains(newPattern)) {
        return seenPatterns.Add(newPattern);
      }
      return false;
    }

    public PatternOrientation BankOrientation {
      get {
        PatternOrientation result = PatternOrientation.Horizontal;
        HashSet<BlockPattern>.Enumerator enumer = Patterns.GetEnumerator();
        bool nonEmpty = enumer.MoveNext();
        Debug.Assert(nonEmpty);
        if (enumer.Current.VerticalStack) {
          result = PatternOrientation.Vertical;
        }
        BlockPattern cur;
        while ((cur = enumer.Current) != null) {
          if (cur.VerticalStack && result == PatternOrientation.Horizontal) {
            return PatternOrientation.Mixed;
          }
          else if (!cur.VerticalStack && result == PatternOrientation.Vertical) {
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

    public int GetNumTotalPatterns() {
      return Patterns.Count;
    }

    public int GetNumSeenPatterns() {
      return seenPatterns.Count;
    }
  }

}
