using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class CozmoBusyPanel : MonoBehaviour {

	[SerializeField] Text text_actionDescription;
	[SerializeField] GameObject panel;

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
		if(RobotEngineManager.instance == null) {
			panel.SetActive(false);
			return;
		}

		Robot robot = RobotEngineManager.instance.current;
		if(robot == null) {
			panel.SetActive(false);
			return;
		}

		if(!robot.isBusy) {
			panel.SetActive(false);
			return;
		}

		panel.SetActive(true);
	}


	public void CancelCurrentActions() {
		if(RobotEngineManager.instance == null) {
			panel.SetActive(false);
			return;
		}
		
		Robot robot = RobotEngineManager.instance.current;
		if(robot == null) {
			panel.SetActive(false);
			return;
		}
		
		if(!robot.isBusy) {
			panel.SetActive(false);
			return;
		}

		robot.CancelAction();

	}

	public void SetDescription(string desc) {
		text_actionDescription.text = desc;
	}
}
