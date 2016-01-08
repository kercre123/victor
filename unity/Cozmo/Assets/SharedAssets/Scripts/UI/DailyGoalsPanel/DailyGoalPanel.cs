using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;

public class DailyGoalPanel : BaseView {

  private List<GoalBadge> _GoalUIBadges;
  [SerializeField]
  private GoalBadge _GoalBadgePrefab;

  private bool _Expanded = true;

	// Use this for initialization
	void Start () {
    _GoalUIBadges = new List<GoalBadge>();
	}
	
	// Update is called once per frame
	void Update () {
	
	}

  public void CreateGoalBadge(string name, int target) {
    GoalBadge newBadge = UIManager.CreateUIElement(_GoalBadgePrefab.gameObject, this.transform).GetComponent<GoalBadge>();
    newBadge.Initialize(name,target,0);
    _GoalUIBadges.Add(newBadge);
    newBadge.Expand(_Expanded);
  }

  public bool Expand {
    get {
      return _Expanded;
    }
    set {
      // TODO: Lerp the width of this from 700 to 350.
      if (_Expanded != value) {
        _Expanded = value;
        for (int i = 0; i < _GoalUIBadges.Count; i++) {
          _GoalUIBadges[i].Expand(value);
        }
      }
    }
  }

  protected override void CleanUp() {
    for (int i = 0; i < _GoalUIBadges.Count; i++) {
      //TODO: Dismiss GoalBadges through themselves
      Destroy(_GoalUIBadges[i].gameObject);
    }
    _GoalUIBadges.Clear();
    
  }
}
