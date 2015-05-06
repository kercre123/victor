using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class CozmoBusyPanel : MonoBehaviour {

	[SerializeField] Text text_actionDescription;
	[SerializeField] GameObject panel;

	Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	public static CozmoBusyPanel instance = null;
	
	void Awake() {
		//enforce singleton
		if(instance != null && instance != this) {
			GameObject.Destroy(instance.gameObject);
			return;
		}
		instance = this;
		DontDestroyOnLoad(gameObject);
	}

	// Update is called once per frame
	void Update () {
		if(robot == null || !robot.isBusy) {
			panel.SetActive(false);
			return;
		}

		panel.SetActive(true);
	}


	public void CancelCurrentActions() {
		if(robot == null || !robot.isBusy) {
			panel.SetActive(false);
			return;
		}

		robot.CancelAction();

	}

	public void SetDescription(string desc) {
		text_actionDescription.text = desc;
	}
}
