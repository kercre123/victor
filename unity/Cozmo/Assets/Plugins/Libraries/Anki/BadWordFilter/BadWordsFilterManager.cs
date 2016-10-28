using UnityEngine;
using System.Text.RegularExpressions;
using System.Collections.Generic;

public class BadWordsFilterManager : MonoBehaviour {

  [SerializeField]
  private TextAsset _BadWordsRegexSource;

  private List<Regex> _BadWordMatches = new List<Regex>();

  private static BadWordsFilterManager _Instance;

  public static BadWordsFilterManager Instance {
    get {
      return _Instance;
    }
  }

  public void Load() {
    string[] splitRegexSource = _BadWordsRegexSource.text.Split(new char[] { '\n' }, System.StringSplitOptions.RemoveEmptyEntries);
    foreach (string regexSource in splitRegexSource) {
      if (regexSource.StartsWith("#")) {
        continue;
      }
      _BadWordMatches.Add(new Regex(regexSource, RegexOptions.IgnorePatternWhitespace | RegexOptions.IgnoreCase));
    }
  }

  private void Awake() {
    _Instance = this;
  }

  private void Start() {
    Load();
  }

  public bool Contains(string testString) {
    foreach (Regex badwordRegex in _BadWordMatches) {
      if (badwordRegex.IsMatch(testString)) {
        return true;
      }
    }
    return false;
  }

}
