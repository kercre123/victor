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
  [SerializeField]
  private ProgressBar _TotalProgressBar;
  [SerializeField]
  private readonly float _ExpandWidth = 800f;
  [SerializeField]
  private readonly float _CollapseWidth = 400f;
  [SerializeField]
  private Transform _GoalContainer;

  private bool _Expanded = true;
  public bool Expand {
    get {
      return _Expanded;
    }
    set {
      // Expand between half and full size
      if (_Expanded != value) {
        _Expanded = value;
        for (int i = 0; i < _GoalUIBadges.Count; i++) {
          _GoalUIBadges[i].Expand(value);
        }
        RectTransform trans = GetComponent<RectTransform>();
        // TODO: DOScaleX does this wrong. Identify a different way to animate the scaling for this element.
        if (_Expanded) {
          trans.sizeDelta = new Vector2(_ExpandWidth, 0.0f);
        }
        else {
          trans.sizeDelta = new Vector2(_CollapseWidth, 0.0f);
        }
      }
    }
  }

	// Use this for initialization
	void Awake () {
    _GoalUIBadges = new List<GoalBadge>();
    // Uncomment this for testing
    /*
    GoalBadge test = CreateGoalBadge("Bond...",4);
    test.SetProgress(0.75f);
    test = CreateGoalBadge("James",3);
    test.SetProgress(2);
    test = CreateGoalBadge("Bond",7);
    test.SetProgress(5);*/
	}
	
	// Update is called once per frame
	void Update () {
	}

  // TODO: Once we have some data class for representing a Goal Type (includes name, listens for the right changes, ect.)
  // replace this with a variant that takes in that class
  public GoalBadge CreateGoalBadge(string name, int target) {
    GoalBadge newBadge = UIManager.CreateUIElement(_GoalBadgePrefab.gameObject, _GoalContainer).GetComponent<GoalBadge>();
    newBadge.Initialize(name,target,0);
    _GoalUIBadges.Add(newBadge);
    newBadge.Expand(_Expanded);
    newBadge.OnProgChanged += UpdateTotalProgress;
    return newBadge;
  }

  // Listens for any goal Badge values you listen to changing.
  // On Change, updates the total progress achieved by all goalbadges.
  public void UpdateTotalProgress() {
    float Total = _GoalUIBadges.Count;
    float Curr = 0.0f;
    for (int i = 0; i < _GoalUIBadges.Count; i++) {
      Curr += _GoalUIBadges[i].Progress;
    }
    _TotalProgressBar.SetProgress(Curr/Total);
  }

  protected override void CleanUp() {
    for (int i = 0; i < _GoalUIBadges.Count; i++) {
      //TODO: Dismiss GoalBadges through themselves
      _GoalUIBadges[i].OnProgChanged -= UpdateTotalProgress;
      Destroy(_GoalUIBadges[i].gameObject);
    }
    _GoalUIBadges.Clear();
  }

}
