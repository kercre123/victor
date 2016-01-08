using UnityEngine;
using System.Collections;
using Cozmo.Util;
using System;
using System.Text;
using Newtonsoft.Json;

[Serializable]
public class StatContainer : IEquatable<StatContainer> {

  [SerializeField]
  [JsonProperty("Stats")]
  private readonly int[] _Stats = new int[(int)CozmoStat.COUNT];

  public int this[CozmoStat stat] {
    get {
      return _Stats[(int)stat];
    }
    set {
      _Stats[(int)stat] = value;
    }
  }

  [JsonIgnore]
  public int Total { 
    get { 
      return _Stats.Sum();
    } 
  }

  public void Add(StatContainer other) {
    _Stats.Add(other._Stats);
  }

  public void Subtract(StatContainer other) {
    _Stats.Subtract(other._Stats);
  }

  public static StatContainer operator+ (StatContainer a, StatContainer b) {
    StatContainer result = new StatContainer();
    result.Add(a);
    result.Add(b);
    return result;
  }

  public static StatContainer operator- (StatContainer a, StatContainer b) {
    StatContainer result = new StatContainer();
    result.Add(a);
    result.Subtract(b);
    return result;
  }

  public static StatContainer operator- (StatContainer a) {
    StatContainer result = new StatContainer();
    result.Subtract(a);
    return result;
  }


  #region IEquatable implementation
  public bool Equals(StatContainer other) {
    if (other == null) {
      return false;
    }
    return _Stats.SequenceEquals(other._Stats);
  }
  #endregion

  public override bool Equals(object obj) {
    return Equals(obj as StatContainer);
  }

  // This isn't great, as there is a good probability of collision. Can't think of anything better though if
  // we want our hashcodes to be equal if Equals returns true
  public override int GetHashCode() {
    int shift = 0;
    int hashcode = 0;
    for (int i = 0; i < _Stats.Length; i++) {
      shift = (i * 7) % 32;
      // roll shift so we don't lose bits
      hashcode ^= (_Stats[i].GetHashCode() << shift) | (_Stats[i].GetHashCode() >> (32 - shift));
    }
    return hashcode;
  }

  public override string ToString() {
    StringBuilder sb = new StringBuilder();

    sb.Append("[StatContainer: ");

    for (int i = 0; i < (int)CozmoStat.COUNT; i++) {
      if (i > 0) {
        sb.Append(", ");
      }
      sb.Append(((CozmoStat)i).ToString());
      sb.Append("=");
      sb.Append(_Stats[i]);
    }
    sb.Append("]");
    return sb.ToString();
  }
}
