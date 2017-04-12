using UnityEngine;
using System.Collections;
using Cozmo.Util;
using System;
using System.Text;
using Newtonsoft.Json;
using System.Collections.Generic;

[Serializable]
[JsonConverter(typeof(StatContainerConverter))]
public class StatContainer : IEquatable<StatContainer> {
  private const int _kCount = (int)Anki.Cozmo.ProgressionStatType.Count;

  public static readonly Anki.Cozmo.ProgressionStatType[] sKeys;

  static StatContainer() {
    sKeys = new Anki.Cozmo.ProgressionStatType[_kCount];
    for (int i = 0; i < _kCount; i++) {
      sKeys[i] = (Anki.Cozmo.ProgressionStatType)i;
    }
  }

  [SerializeField]
  private int[] _Stats = new int[_kCount];

  public int this[Anki.Cozmo.ProgressionStatType stat] {
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

  public void Set(StatContainer other) {
    other._Stats.CopyTo(_Stats, 0);
  }

  public static StatContainer operator+(StatContainer a, StatContainer b) {
    StatContainer result = new StatContainer();
    result.Add(a);
    result.Add(b);
    return result;
  }

  public static StatContainer operator-(StatContainer a, StatContainer b) {
    StatContainer result = new StatContainer();
    result.Add(a);
    result.Subtract(b);
    return result;
  }

  public static StatContainer operator-(StatContainer a) {
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

    for (int i = 0; i < _kCount; i++) {
      if (i > 0) {
        sb.Append(", ");
      }
      sb.Append(((Anki.Cozmo.ProgressionStatType)i).ToString());
      sb.Append("=");
      sb.Append(_Stats[i]);
    }
    sb.Append("]");
    return sb.ToString();
  }


  #region IDicionary Interface

  public void Add(Anki.Cozmo.ProgressionStatType key, int value) {
    this[key] = value;
  }

  public bool ContainsKey(Anki.Cozmo.ProgressionStatType key) {
    int keyInt = (int)key;
    return (keyInt >= 0 && keyInt < _Stats.Length);
  }

  public bool Remove(Anki.Cozmo.ProgressionStatType key) {
    // We can't actually remove an entry, assume
    // they just want to clear it
    if (ContainsKey(key)) {
      this[key] = 0;
      return true;
    }
    return false;
  }

  public bool TryGetValue(Anki.Cozmo.ProgressionStatType key, out int value) {
    if (ContainsKey(key)) {
      value = this[key];
      return true;
    }
    else {
      value = 0;
      return false;
    }
  }

  public ICollection<Anki.Cozmo.ProgressionStatType> Keys {
    get {
      return sKeys;
    }
  }

  public ICollection<int> Values {
    get {
      return _Stats;
    }
  }

  public void Add(KeyValuePair<Anki.Cozmo.ProgressionStatType, int> item) {
    this[item.Key] = item.Value;
  }

  public void Clear() {
    Array.Clear(_Stats, 0, _Stats.Length);
  }

  public bool Contains(KeyValuePair<Anki.Cozmo.ProgressionStatType, int> item) {
    return ContainsKey(item.Key) && this[item.Key] == item.Value;
  }

  public void CopyTo(KeyValuePair<Anki.Cozmo.ProgressionStatType, int>[] array, int arrayIndex) {
    for (int i = 0; i < _Stats.Length && i < array.Length - arrayIndex; i++) {
      array[i + arrayIndex] = new KeyValuePair<Anki.Cozmo.ProgressionStatType, int>((Anki.Cozmo.ProgressionStatType)i, _Stats[i]);
    }
  }

  public bool Remove(KeyValuePair<Anki.Cozmo.ProgressionStatType, int> item) {
    if (Contains(item)) {
      this[item.Key] = 0;
      return true;
    }
    return false;
  }

  public int Count {
    get {
      return _kCount;
    }
  }

  public bool IsReadOnly {
    get {
      return false;
    }
  }

  public IEnumerator<KeyValuePair<Anki.Cozmo.ProgressionStatType, int>> GetEnumerator() {
    for (int i = 0; i < _Stats.Length; i++) {
      yield return new KeyValuePair<Anki.Cozmo.ProgressionStatType, int>((Anki.Cozmo.ProgressionStatType)i, _Stats[i]);
    }
  }

  #endregion
}
