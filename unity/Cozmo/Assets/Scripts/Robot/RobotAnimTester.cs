using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using U2G = Anki.Cozmo.U2G;

/// <summary>
/// simple component for directly previewing animations on cozmo
/// </summary>
public class RobotAnimTester : MonoBehaviour {
	
	[SerializeField] RectTransform anchor = null;
	[SerializeField] ComboBox combo_selectAnim = null;
	[SerializeField] Toggle toggle_loop = null;

	private string[] animNames = null;

	private U2G.PlayAnimation PlayAnimationMessage = new U2G.PlayAnimation();

	void Awake() {
		if(RobotEngineManager.instance == null) {
			gameObject.SetActive(false);
			anchor.gameObject.SetActive(false);
			return;
		}

		anchor.gameObject.SetActive(true);

		animNames = RobotEngineManager.instance.robotAnimationNames.ToArray();
		combo_selectAnim.AddItems(animNames);
	}

	public void PlayAnimation () {
		if(RobotEngineManager.instance == null) return;
		if(RobotEngineManager.instance.current == null) return;
		if(animNames == null) return;
		if(combo_selectAnim == null) return;
		if(toggle_loop == null) return;

		string anim = animNames[combo_selectAnim.SelectedIndex];

		if(CozmoBusyPanel.instance != null) {
			CozmoBusyPanel.instance.SetMute(true);
			CozmoBusyPanel.instance.SetDescription("Cozmo playing animation" + anim + ".");
		}

		if(RobotEngineManager.instance.current.isBusy) RobotEngineManager.instance.current.CancelAction();

		PlayAnimationMessage.animationName = anim;
		PlayAnimationMessage.numLoops = (uint)(toggle_loop.isOn ? 0 : 1);
		PlayAnimationMessage.robotID = RobotEngineManager.instance.current.ID;
		RobotEngineManager.instance.Message.PlayAnimation = PlayAnimationMessage;
		RobotEngineManager.instance.SendMessage();
	}
}
