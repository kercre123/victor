using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;


[System.Serializable]
public class ActionSlider {
	public Slider slider;
	[SerializeField] private GameObject highlight;
	[SerializeField] private Text text;
	[SerializeField] private Image image;
	[SerializeField] private Image[] hints;
	
	public bool Pressed { get; private set; }
	
	public ActionButton currentAction { get; private set; }

	public void SetMode(ActionButton button, bool down) {
		//Debug.Log("ActionSlider.SetMode("+mode+", "+down+") modes("+(modes != null ? modes.Count.ToString() : "null")+") index("+selectedIndex+")");
		if( button == null ) {
			Debug.LogError( "Slider was given a current action of null" );
			return;
		}

		currentAction = button;
		Pressed = down;

		if(button.mode != ActionButton.Mode.DISABLED) {
			highlight.SetActive(Pressed);
			text.gameObject.SetActive(Pressed);
		}
		
		text.text = currentAction.text.text;
		image.sprite = currentAction.image.sprite;

		SetHints();
	}

	public void SetHints()
	{
		for(int i = 0; i < hints.Length && ActionSliderPanel.instance != null && i < ActionSliderPanel.instance.actionButtons.Length; ++i) {
			if(hints[i] != null) {
				if(hints[i].sprite != ActionSliderPanel.instance.actionButtons[i].image.sprite) {
					hints[i].sprite = ActionSliderPanel.instance.actionButtons[i].image.sprite;
				}
				hints[i].gameObject.SetActive(!Pressed && ActionSliderPanel.instance.actionButtons[i].mode != ActionButton.Mode.DISABLED);
			}
		}
	}
}

public class ActionSliderPanel : ActionPanel
{
	[SerializeField] public ActionSlider actionSlider = null;
	[SerializeField] protected AudioClip slideInSound;
	[SerializeField] protected AudioClip slideOutSound;

	protected ActionButton bottomAction { get { return actionButtons[0]; } }
	protected ActionButton topAction { get { return actionButtons[1]; } }
	protected ActionButton centerAction { get { return actionButtons[2]; } }

	//public bool Engaged { get { return actionSlider != null ? actionSlider.Pressed : false; } }

	protected bool upLastFrame = true;
	protected bool downLastFrame = false;
	protected ActionButton.Mode lastMode = ActionButton.Mode.DISABLED;

	protected override void Awake() {
		base.Awake();
		
		//actionSlider.ClaimOwnership(this);
		actionSlider.slider.value = 0f;
		actionSlider.SetMode(centerAction, false);
	}
	
	protected void Update() {
		lastMode = actionSlider.currentAction.mode;

		if(actionSlider != null) actionSlider.SetHints();

		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			return;
		}
		
		robot = RobotEngineManager.instance.current;

		if(!actionSlider.Pressed) {
			if(!upLastFrame) actionSlider.currentAction.OnRelease();

			upLastFrame = true;
			downLastFrame = false;
		}
		else {
			if(!downLastFrame) actionSlider.currentAction.OnPress();

			downLastFrame = true;
			upLastFrame = false;

			RefreshSliderMode();
		}
	}

	public void ToggleInteract(bool val) {
		actionSlider.SetMode(actionSlider.currentAction, val);
		
		if(!val) {
			actionSlider.slider.value = 0f;
		}
		
		//Debug.Log("ToggleInteract("+val+")");
	}

	private void RefreshSliderMode() {
		ActionButton currentButton = centerAction;

		if(actionSlider.slider.value < -0.5f && bottomAction.mode != ActionButton.Mode.DISABLED) {
			currentButton = bottomAction;

			float minZ = float.MaxValue;
			for(int i=0;i<robot.selectedObjects.Count && i<2;i++) {
				if(minZ < robot.selectedObjects[i].WorldPosition.z) continue;
				minZ = robot.selectedObjects[i].WorldPosition.z;
				robot.targetLockedObject = robot.selectedObjects[i];
			}
		}
		else if(actionSlider.slider.value > 0.5f && topAction.mode != ActionButton.Mode.DISABLED) {
			currentButton = topAction;
			
			float maxZ = float.MinValue;
			for(int i=0;i<robot.selectedObjects.Count && i<2;i++) {
				if(maxZ > robot.selectedObjects[i].WorldPosition.z) continue;
				maxZ = robot.selectedObjects[i].WorldPosition.z;
				robot.targetLockedObject = robot.selectedObjects[i];
			}
			
			//Debug.Log("RefreshSliderMode index = index2("+index2+")");
		}
		
		if(currentButton.mode != lastMode && currentButton.mode != ActionButton.Mode.TARGET) {
			SlideInSound();
		}
		else if(currentButton.mode != lastMode) {
			SlideOutSound();
		}
		//Debug.Log("RefreshSliderMode currentMode("+currentMode+","+currentButton+")");
		actionSlider.SetMode(currentButton, actionSlider.Pressed);
	}
	
	/*private void InitiateAssistedInteraction() {
		switch(actionSlider.ActionButton.Mode) {
			case ActionButton.Mode.TARGET:
				//do auto-targeting here!
				break;
		}
	}*/

	public void SlideInSound()
	{
		audio.volume = 1f;
		audio.PlayOneShot( slideInSound, 1f );
	}
	
	public void SlideOutSound()
	{
		audio.volume = 1f;
		audio.PlayOneShot( slideOutSound, 1f );
	}
}
