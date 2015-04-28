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
		
		if(button.mode == ActionButtonMode.DISABLED) return;

		highlight.SetActive(Pressed);
		
		text.gameObject.SetActive(Pressed);

		text.text = currentAction.text.text;
		image.sprite = currentAction.image.sprite;
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

	public bool Engaged { get { return actionSlider != null ? actionSlider.Pressed : false; } }

	protected bool interactLastFrame = false;
	protected ActionButtonMode lastMode = ActionButtonMode.DISABLED;

	protected override void Awake() {
		base.Awake();
		
		//actionSlider.ClaimOwnership(this);
		actionSlider.slider.value = 0f;
		actionSlider.SetMode(centerAction, true);
	}
	
	protected void Update() {
		lastMode = actionSlider.currentAction.mode;

		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			return;
		}
		
		robot = RobotEngineManager.instance.current;

		if(!actionSlider.Pressed) {
			
			if(interactLastFrame) {
				DoReleaseAction();
			}

			robot.selectedObjects.Clear();
			interactLastFrame = false;
			robot.targetLockedObject = null;
			
			actionSlider.SetMode(centerAction, false);

			return;
		}

		RefreshSliderMode();
		
		//if we have a new target, let's see if our current slider mode wants to initiate an interaction
		/*if(robot.selectedObjects.Count > 0 && !robot.isBusy) {
			
			if(lastMode != actionSlider.actionButtonMode || robot.selectedObjects.Count != robot.lastSelectedObjects.Count ||
			   robot.selectedObjects[0] != robot.lastSelectedObjects[0] ) {
				
				InitiateAssistedInteraction();
			}
		}*/
			
		interactLastFrame = true;
		
		//Debug.Log("CozmoVision4.Update_2 selectedObjects("+robot.selectedObjects.Count+") isBusy("+robot.isBusy+")");
	}

	public void ToggleInteract(bool val) {
		actionSlider.SetMode(actionSlider.currentAction, val);
		
		if(!val) {
			actionSlider.slider.value = 0f;
		}
		
		//Debug.Log("ToggleInteract("+val+")");
	}
	
	private void RefreshSliderMode() {
		GameActions.instance.SetActionButtons();

		ActionButton currentButton = centerAction;

		if(actionSlider.slider.value < -0.5f && bottomAction.mode != ActionButtonMode.DISABLED) {
			currentButton = bottomAction;

			float minZ = float.MaxValue;
			for(int i=0;i<robot.selectedObjects.Count && i<2;i++) {
				if(minZ < robot.selectedObjects[i].WorldPosition.z) continue;
				minZ = robot.selectedObjects[i].WorldPosition.z;
				robot.targetLockedObject = robot.selectedObjects[i];
			}
		}
		else if(actionSlider.slider.value > 0.5f && topAction.mode != ActionButtonMode.DISABLED) {
			currentButton = topAction;
			
			float maxZ = float.MinValue;
			for(int i=0;i<robot.selectedObjects.Count && i<2;i++) {
				if(maxZ > robot.selectedObjects[i].WorldPosition.z) continue;
				maxZ = robot.selectedObjects[i].WorldPosition.z;
				robot.targetLockedObject = robot.selectedObjects[i];
			}
			
			//Debug.Log("RefreshSliderMode index = index2("+index2+")");
		}
		
		if(currentButton.mode != lastMode && currentButton.mode != ActionButtonMode.TARGET) {
			SlideInSound();
		}
		else if(currentButton.mode != lastMode) {
			SlideOutSound();
		}
		//Debug.Log("RefreshSliderMode currentMode("+currentMode+","+currentButton+")");
		actionSlider.SetMode(currentButton, actionSlider.Pressed);
	}
	
	/*private void InitiateAssistedInteraction() {
		switch(actionSlider.actionButtonMode) {
			case ActionButtonMode.TARGET:
				//do auto-targeting here!
				break;
		}
	}*/
	
	private void DoReleaseAction() {
		if( actionSlider.currentAction.doActionOnRelease ) actionSlider.currentAction.button.onClick.Invoke();
	}

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
