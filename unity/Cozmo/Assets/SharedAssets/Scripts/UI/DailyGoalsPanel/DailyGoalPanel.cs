using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;

public class DailyGoalPanel : BaseView {

  // TODO: Position GoalBadges procedurally as they are added to the list.
  private List<GoalBadge> _GoalUIBadges;
  [SerializeField]
  private GoalBadge _GoalBadgePrefab;

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

  }

  protected override void CleanUp() {
    for (int i = 0; i < _GoalUIBadges.Count; i++) {
      //TODO: Dismiss GoalBadges through themselves
      Destroy(_GoalUIBadges[i].gameObject);
    }
    _GoalUIBadges.Clear();
    
  }
}
