using System;
using Anki.Cozmo;
using UnityEngine;
using System.Collections.Generic;
using System.Linq;

[System.Serializable]
public struct StatBitMask {
  [SerializeField]
  private int _Mask;

  private StatBitMask(int mask) {    
    //clear everything above the last stat
    // this lets us construct the mask with -1
    _Mask = mask % (1 << (int)ProgressionStatType.Count);
  }

  public StatBitMask(ProgressionStatType stat) {
    _Mask = 0;
    this[stat] = true;    
  }

  public StatBitMask(params ProgressionStatType[] stats) {
    _Mask = 0;
    foreach (var stat in stats) {
      this[stat] = true;
    }
  }

  public bool this[ProgressionStatType stat] {
    get {
      return (_Mask & (1 << (int)stat)) != 0;
    }
    set {
      int offset = (1 << (int)stat);
      if(value) {
        _Mask |= offset;
      }
      else {
        _Mask &= ~offset;
      }
    }
  }

  public static readonly StatBitMask None = new StatBitMask(0);
  public static readonly StatBitMask All = new StatBitMask(-1);

  public static StatBitMask operator|(StatBitMask a, StatBitMask b) {
    return new StatBitMask(a._Mask | b._Mask);
  }

  public static StatBitMask operator^(StatBitMask a, StatBitMask b) {
    return new StatBitMask(a._Mask ^ b._Mask);
  }

  public static StatBitMask operator&(StatBitMask a, StatBitMask b) {
    return new StatBitMask(a._Mask & b._Mask);
  }

  public static StatBitMask operator~(StatBitMask a) {
    return new StatBitMask(~a._Mask);
  }

  public static bool operator==(StatBitMask a, StatBitMask b) {
    return a._Mask == b._Mask;
  }

  public static bool operator!=(StatBitMask a, StatBitMask b) {
    return a._Mask != b._Mask;
  }

  public static implicit operator StatBitMask(ProgressionStatType stat) {
    return new StatBitMask(stat);
  }

  public static StatBitMask FromStatContainer(StatContainer stats) {
    StatBitMask bitMask = None;
    for (int i = 0; i < (int)ProgressionStatType.Count; i++) {
      var statType = (ProgressionStatType)i;
      bitMask[statType] = stats[statType] > 0;
    }
    return bitMask;
  }


  public override bool Equals(object obj) {
    if (!(obj is StatBitMask)) {
      return false;
    }
    return this == ((StatBitMask)obj);
  }

  public override int GetHashCode() {
    return _Mask;
  }

  public bool IsEmpty {
    get {
      return _Mask == 0;
    }
  }

  public int Count {
    get {
      if (IsEmpty) {
        return 0;
      }
      int count = 0;
      for (int i = 0; i < (int)ProgressionStatType.Count; i++) {
        if (this[(ProgressionStatType)i]) {
          count++;
        }
      }
      return count;
    }
  }

  public ProgressionStatType Random() {
    int count = Count;

    if (count == 0) {
      return ProgressionStatType.Count;
    }

    int r = UnityEngine.Random.Range(0, count);

    for (int i = 0; i < (int)ProgressionStatType.Count; i++) {
      if ((_Mask & (1 << i)) != 0) {
        if (r == 0) {
          return (ProgressionStatType)i;
        }
        r--;
      }
    }

    return ProgressionStatType.Count;
  }

  public IEnumerable<ProgressionStatType> GetStats() {
    for (int i = 0; i < (int)ProgressionStatType.Count; i++) {
      if ((_Mask & (1 << i)) != 0) {
        yield return (ProgressionStatType)i;
      }
    }
  }

  public override string ToString() {
    return "[StatBitMask: ("+_Mask+") " +
           string.Join(", ", GetStats()
                 .Select(x => x.ToString())
                 .ToArray()) + "]";
  }
}

