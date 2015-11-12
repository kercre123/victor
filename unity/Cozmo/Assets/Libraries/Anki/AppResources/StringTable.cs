using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

// http://stackoverflow.com/a/2679857/217431
public static class DictionaryExtensions {
  // Works in C#3/VS2008:
  // Returns a new dictionary of this ... others merged leftward.
  // Keeps the type of 'this', which must be default-instantiable.
  // Example:
  //   result = map.MergeLeft(other1, other2, ...)
  public static T MergeLeft<T,K,V>(this T me, params IDictionary<K,V>[] others)
    where T : IDictionary<K,V>, new() {
    T newMap = new T();
    foreach (IDictionary<K,V> src in
      (new List<IDictionary<K,V>> { me }).Concat(others)) {
      // ^-- echk. Not quite there type-system.
      foreach (KeyValuePair<K,V> p in src) {
        newMap[p.Key] = p.Value;
      }
    }
    return newMap;
  }
}

namespace Anki.AppResources {
  public sealed class StringTable {
    private Dictionary<string, string> _StringMap;

    public StringTable() {
      _StringMap = new Dictionary<string, string>();
    }

    public StringTable(Dictionary<string,string> stringMap) {
      _StringMap = stringMap;
    }

    public void MergeEntriesFromStringTable(StringTable other) {
      _StringMap = _StringMap.MergeLeft(other._StringMap);
    }

    public bool TryGetString(string key, out string value) {
      return _StringMap.TryGetValue(key, out value);
    }

    public string SafeFetch(string key) {
      if (null == _StringMap) {
        return key;
      }

      string v;
      if (!TryGetString(key, out v)) {
        v = key;
      }

      return v;
    }

    public string this[string key] {
      get {
        return SafeFetch(key);
      }
    }

    public static StringTable LoadStringsFromSmartlingJSONFile(string smartlingJSONFile) {

      System.IO.StreamReader reader = System.IO.File.OpenText(smartlingJSONFile);
      string jsonText = Anki.Util.ReadUnicodeEscapedString(reader);
      reader.Close();

      JSONObject stringMapJSON = new JSONObject(jsonText);

      List<string> keys = stringMapJSON.keys;
      int keyCount = keys.Count;

      Dictionary<string, string> stringMap = new Dictionary<string, string>();

      for (int i = 0; i < keyCount; ++i) {
        string k = keys[i];
        if ("smartling" == k) {
          continue;
        }
        JSONObject entry = stringMapJSON.GetField(k);
        string s = entry.GetField("translation").str;
        stringMap[k] = s;
      }

      StringTable st = new StringTable(stringMap);

      return st;
    }

    public void DebugDump() {
      foreach (var entry in _StringMap) {
        string key = entry.Key;
        string value = entry.Value;

        StringBuilder sb = new StringBuilder();
        sb.Append(String.Format("  {0} => {1}\n", key, value));

        UnityEngine.Debug.Log(sb.ToString());
      }
    }
  }
  // class StringTable
}
// namespace Anki.AppResources