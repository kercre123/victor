using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class BadgeData
{
  public object key;
  public List<string> tags;
}

public class BadgeManager : MonoBehaviour {

  private static BadgeManager _instance;
  public static BadgeManager Instance
  {
    get {
      if (_instance == null)
      {
        Debug.LogError("Don't access this until Start!");
      }
      return _instance;
    }
    private set {
      if (_instance != null)
      {
        Debug.LogError("There shouldn't be more than one UIManager");
      }
      _instance = value;
    }
  }

  public static bool TryAddBadge(object badgeKey, List<string> badgeTags) {
    return _instance.TryAddBadgeInternal (badgeKey, badgeTags);
  }

  public static bool TryRemoveBadge (object badgeKey) {
    return _instance.TryRemoveBadgeInternal(badgeKey);
  }

  public static bool DoesBadgeExistForKey(object key) {
    return _instance.DoesBadgeExistForKeyInternal(key);
  }

  public static int NumBadgesWithTag(string tag) {
    return _instance.NumBadgesWithTagInternal(tag);
  }
  
  private Dictionary<object, List<string>> _currentBadges;
  private Dictionary<string, int> _numBadgesByTag;
  
  private void Awake()
  {
    _instance = this;
    _currentBadges = new Dictionary<object, List<string>> ();
    _numBadgesByTag = new Dictionary<string, int> ();
  }

  private bool TryAddBadgeInternal(object badgeKey, List<string> badgeTags) {
    bool badgeAdded = false;
    // Add badge and tags
    if (!_currentBadges.ContainsKey (badgeKey)) {
      _currentBadges.Add (badgeKey, badgeTags);
      badgeAdded = true;

      // Update the number of alerts per tag
      foreach (string tag in badgeTags) {
        if(_numBadgesByTag.ContainsKey(tag)) {
          _numBadgesByTag[tag]++;
        } else {
          _numBadgesByTag.Add(tag, 1);
        }
      }
    }
    return badgeAdded;
  }
  
  private bool TryRemoveBadgeInternal (object badgeKey) {
    bool badgeRemoved = false;
    if (_currentBadges.ContainsKey (badgeKey)) {
      // Update the number of alerts per tag
      foreach(string tag in _currentBadges[badgeKey])
      {
        if(_numBadgesByTag.ContainsKey(tag)){
          _numBadgesByTag[tag]--;
          if(_numBadgesByTag[tag] <= 0)
          {
            _numBadgesByTag.Remove(tag);
          }
        } else {
          Debug.LogError("Tried to decrement a tag that didn't exist! tag=" + tag);
        }
      }

      _currentBadges.Remove(badgeKey);
      badgeRemoved = true;
    }
    return badgeRemoved;
  }
  
  private bool DoesBadgeExistForKeyInternal(object key) {
    return _currentBadges.ContainsKey(key);
  }
  
  private int NumBadgesWithTagInternal(string tag) {
    int numBadges = 0;
    _numBadgesByTag.TryGetValue(tag, out numBadges);
    return numBadges;
  }
}
