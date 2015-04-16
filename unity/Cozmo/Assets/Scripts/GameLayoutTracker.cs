using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using Anki.Cozmo;
using G2U = Anki.Cozmo.G2U;
using U2G = Anki.Cozmo.U2G;

public class GameLayoutTracker : MonoBehaviour {

	[SerializeField] List<GameLayout> layouts;
	[SerializeField] Text title;
	[SerializeField] Text instructions;
	[SerializeField] Button buttonPrevLayout;
	[SerializeField] Button buttonNextLayout;
	[SerializeField] Button buttonStartBuild;
	[SerializeField] Button buttonValidateBuild;
	[SerializeField] ScreenMessage screenMessage = null;

	public bool Validated { get; private set; }

	List<GameLayout> filteredLayouts;
	string gameTypeFilter = null;

	int currentLayoutIndex = 0;
	int currentPageIndex = 0;

	bool dirty = false;
	Robot robot = null;

	//uint currentObjectType = 0;

	GameLayout currentLayout {
		get {
			if(filteredLayouts == null) return null;
			if(currentLayoutIndex < 0) return null;
			if(currentLayoutIndex >= filteredLayouts.Count) return null;
			return filteredLayouts[currentLayoutIndex];
		}
	}

	void Awake() {
		for(int i=0; i<layouts.Count; i++) {
			layouts[i].Initialize();
		}
	}

	void OnEnable () {
		currentLayoutIndex = 0;
		currentPageIndex = 0;

		Validated = false;

		for(int i=0; i<layouts.Count; i++) {
			layouts[i].gameObject.SetActive(false);
		}

		filteredLayouts = layouts.FindAll(x => string.IsNullOrEmpty(gameTypeFilter) || x.gameTypeFilter == gameTypeFilter);
		currentLayoutIndex = Mathf.Clamp(currentLayoutIndex, 0, filteredLayouts.Count-1);
		//Debug.Log("OnEnable filterType("+(gameTypeFilter != null ? gameTypeFilter : "null")+" filteredLayouts("+filteredLayouts.Count+")");

		RefreshLayout();

		if(RobotEngineManager.instance != null) {
			RobotEngineManager.instance.SuccessOrFailure += SuccessOrFailure;
			robot = RobotEngineManager.instance.current;
		}

	}

	void Update () {
		if(RobotEngineManager.instance == null) return;
		if(RobotEngineManager.instance.current == null) return;

		robot = RobotEngineManager.instance.current;

		if(dirty) RefreshLayout();
		dirty = false;
	}

	void OnDisable() {
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure -= SuccessOrFailure;
	}

	void RefreshLayout () {

		for(int i=0; i<filteredLayouts.Count; i++) {
			filteredLayouts[i].gameObject.SetActive(currentLayoutIndex == i);
		}

		GameLayout layout = currentLayout;

		title.text = layout.title;

		for(int i=0; i<layout.blocks.Length; i++) {
			if(currentPageIndex == 0) {
				layout.blocks[i].Hidden = false;
				layout.blocks[i].Highlighted = false;
				layout.blocks[i].Validated = true;
			}
			else {
				bool stepValidated = false;
				//layout.cube[i].Hidden = i >= (currentPageIndex-1);
				layout.blocks[i].Highlighted = !stepValidated;

				//dmd2do make cozmo confirm this cubes existance and position
				layout.blocks[i].Validated = stepValidated;
			}
		}

		if(instructions != null) {
			if(currentPageIndex <= 0) {
				instructions.text = "Press to start building this game's layout!";
			}
			else if(currentPageIndex >= layout.blocks.Length+1) {
				instructions.text = "Press to validate the build and start playing!";
			}
			else {
				instructions.text = "Build this layout!" ;
			}
		}

		if(buttonPrevLayout != null) buttonPrevLayout.interactable = currentLayoutIndex > 0;
		if(buttonNextLayout != null) buttonNextLayout.interactable = currentLayoutIndex < filteredLayouts.Count-1;

		if(buttonStartBuild != null) buttonStartBuild.gameObject.SetActive(currentPageIndex == 0);
		if(buttonValidateBuild != null) buttonValidateBuild.gameObject.SetActive(currentPageIndex == 2);

	}

	public void SetLayoutForGame(string filterType) {
		gameTypeFilter = filterType;
		
		filteredLayouts = layouts.FindAll(x => string.IsNullOrEmpty(filterType) || x.gameTypeFilter == filterType);

		//Debug.Log("SetLayoutForGame filterType("+filterType.ToString()+" filteredLayouts("+filteredLayouts.Count+")");

		currentLayoutIndex = 0;
		currentPageIndex = 0;
		Validated = false;

		RefreshLayout();
	}

	public void NextLayout() {
		currentLayoutIndex++;
		if(currentLayoutIndex >= filteredLayouts.Count) currentLayoutIndex = 0;
		currentPageIndex = 0;
		Validated = false;
		RefreshLayout();
	}
	
	public void PreviousLayout() {
		currentLayoutIndex--;
		if(currentLayoutIndex < 0) currentLayoutIndex = filteredLayouts.Count - 1;
		currentPageIndex = 0;
		Validated = false;
		RefreshLayout();
	}

	public void StartBuildingLayout() {
		currentPageIndex = 1;
		RefreshLayout();
	}
	
	public void ValidateLayoutAndStartGame() {
		currentPageIndex = 2;
		RefreshLayout();
	}

	public void ValidateBuild() {
		Validated = true;
	}

	private void SuccessOrFailure(bool success, ActionCompleted action_type) {
		if(success) {
			dirty = true; //for now let's just refresh whenever an action finishes
			switch(action_type) {
				case ActionCompleted.UNKNOWN://	ACTION_UNKNOWN = -1,
					break;
				case ActionCompleted.DRIVE_TO_POSE:
					break;
				case ActionCompleted.DRIVE_TO_OBJECT:
					break;
				case ActionCompleted.DRIVE_TO_PLACE_CARRIED_OBJECT:
					break;
				case ActionCompleted.TURN_IN_PLACE:
					break;
				case ActionCompleted.MOVE_HEAD_TO_ANGLE:
					break;
				case ActionCompleted.PICKUP_OBJECT_LOW:
				case ActionCompleted.PICKUP_OBJECT_HIGH:
					break;
				case ActionCompleted.PLACE_OBJECT_LOW:
				case ActionCompleted.PLACE_OBJECT_HIGH:
					break;
				case ActionCompleted.CROSS_BRIDGE:
				case ActionCompleted.ASCEND_OR_DESCEND_RAMP:
				case ActionCompleted.TRAVERSE_OBJECT:
				case ActionCompleted.DRIVE_TO_AND_TRAVERSE_OBJECT:
				case ActionCompleted.FACE_OBJECT:
				case ActionCompleted.PLAY_ANIMATION:
				case ActionCompleted.PLAY_SOUND:
				case ActionCompleted.WAIT:
					break;
			}
		}
		else {
			screenMessage.ShowMessageForDuration("Cozmo ran into difficulty, let's try that again.", 5f, Color.yellow);
		}
	}

	private bool CozmoKnowsAboutObjectOfType(int objType) {

		for(int i=0; i<robot.knownObjects.Count; i++) {



		}


		return false;
	}

}
