using System;

[Serializable]
public class LocalizedString {
  public string Key;

  public static implicit operator string(LocalizedString locString) {
    return locString.ToString();
  }

  public override string ToString() {
    return Localization.Get(Key);
  }
}