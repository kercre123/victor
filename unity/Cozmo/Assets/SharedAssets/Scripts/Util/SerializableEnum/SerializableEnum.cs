// from http://www.scottwebsterportfolio.com/blog/2015/3/29/enum-serialization-in-unity

using UnityEngine;
using System;

[Serializable]
public class SerializableEnum<T> where T : struct, IConvertible {
  public T Value {
    get { return (T)Enum.Parse(typeof(T), _EnumValueAsString); }
    set { _EnumValueAsString = value.ToString(); }
  }

  [SerializeField]
  private string _EnumValueAsString;

  [SerializeField]
  private T _EnumValue;
}
