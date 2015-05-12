using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class CozmoStatus : MonoBehaviour {

	[SerializeField] LayoutBlock2d carriedBlock2d;
	[SerializeField] ChangeCubeModeButton button_change;

	Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	void Update () {
		if(robot == null) return;

		if(robot.carryingObject == null) {
			carriedBlock2d.gameObject.SetActive(false);
			button_change.gameObject.SetActive(false);
		}
		else {
			carriedBlock2d.Initialize(robot.carryingObject);
			carriedBlock2d.gameObject.SetActive(true);
			button_change.gameObject.SetActive(robot.carryingObject.isActive);
		}
	}
}
