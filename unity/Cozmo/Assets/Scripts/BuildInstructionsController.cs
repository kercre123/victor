using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using Anki.Cozmo;

public class BuildInstructionsController : MonoBehaviour {

	public enum BuildStepPhase
	{
		REQUESTING,
		CHANGING,
		PLACING,
		PLACED,
		NUMSTATES
	};

	[SerializeField] List<BuildInstructions> layouts;
	[SerializeField] Text title;
	[SerializeField] Text instructions;
	[SerializeField] Text note;
	[SerializeField] Button buttonPrevLayout;
	[SerializeField] Button buttonNextLayout;
	[SerializeField] Button buttonPrevStep;
	[SerializeField] Button buttonNextStep;
	[SerializeField] Button buttonStartBuild;
	[SerializeField] Button buttonValidateBuild;
	[SerializeField] ScreenMessage screenMessage = null;

	public bool Validated { get; private set; }

	List<BuildInstructions> filteredLayouts;
	string gameTypeFilter = null;

	int currentLayoutIndex = 0;
	int currentPageIndex = 0;

	BuildStepPhase currentPhase = BuildStepPhase.REQUESTING;
	bool dirty = false;
	Robot robot = null;

	//framewise inputs
//	bool iObjectPickedUp = false;
	bool iObjectPlaced = false;
//	bool iObjectSeen = false;

	BuildInstructions currentLayout {
		get {
			if(filteredLayouts == null) return null;
			if(currentLayoutIndex < 0) return null;
			if(currentLayoutIndex >= filteredLayouts.Count) return null;
			return filteredLayouts[currentLayoutIndex];
		}
	}

	BuildInstructionStep currentStep {
		get {

			BuildInstructions layout = currentLayout;
			if(layout == null) return null;
			int stepIndex = currentPageIndex-1;
			if(stepIndex < 0) return null;
			if(stepIndex >= layout.steps.Length) return null;
			return layout.steps[stepIndex];
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
		currentPhase = BuildStepPhase.REQUESTING;
		//Debug.Log("OnEnable filterType("+(gameTypeFilter != null ? gameTypeFilter : "null")+" filteredLayouts("+filteredLayouts.Count+")");

		note.text = "";
		RefreshPage();

		if(RobotEngineManager.instance != null) {
			RobotEngineManager.instance.SuccessOrFailure += SuccessOrFailure;
			robot = RobotEngineManager.instance.current;
		}

	}

	void Update () {
		if(RobotEngineManager.instance == null) return;
		if(RobotEngineManager.instance.current == null) return;

		robot = RobotEngineManager.instance.current;

		BuildStepPhase nextPhase = GetNextBuildStepPhase();

		if(currentPhase != nextPhase) {
			ExitPhase();
			currentPhase = nextPhase;
			EnterPhase();
		}
		else {
			UpdatePhase();
		}

		if(dirty) RefreshPage();
		dirty = false;

		ResetFramewiseInputs();
	}

	void OnDisable() {
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure -= SuccessOrFailure;
	}

	void RefreshPage () {

		for(int i=0; i<filteredLayouts.Count; i++) {
			filteredLayouts[i].gameObject.SetActive(currentLayoutIndex == i);
		}

		BuildInstructions layout = currentLayout;

		title.text = layout.title;

		for(int i=0; i<layout.steps.Length; i++) {
			if(currentPageIndex == 0) {
				layout.steps[i].cube.Hidden = false;
				layout.steps[i].cube.Highlighted = false;
				layout.steps[i].cube.Validated = true;
			}
			else {
				layout.steps[i].cube.Hidden = i >= (currentPageIndex-1);
				layout.steps[i].cube.Highlighted = i == (currentPageIndex-1);

				//dmd2do make cozmo confirm this cubes existance and position
				layout.steps[i].cube.Validated = false;
			}
		}

		if(instructions != null) {
			if(currentPageIndex <= 0) {
				instructions.text = "Press to start building this game's layout!";
			}
			else if(currentPageIndex >= layout.steps.Length+1) {
				instructions.text = "Press to validate the build and start playing!";
			}
			else {
				BuildInstructionStep step = layout.steps[currentPageIndex-1];
				instructions.text = "Step " + currentPageIndex + ": Place a " + step.cube.GetPropTypeName() + " " + step.cube.GetPropFamilyName() + "." ;

				if(!string.IsNullOrEmpty(step.explicitInstruction)) {
					note.text = step.explicitInstruction;
				}
				else {
					note.text = "";
				}
			}

		}

		if(buttonPrevLayout != null) buttonPrevLayout.interactable = currentLayoutIndex > 0;
		if(buttonNextLayout != null) buttonNextLayout.interactable = currentLayoutIndex < filteredLayouts.Count-1;
		if(buttonPrevStep != null) buttonPrevStep.interactable = currentPageIndex > 0;
		if(buttonNextStep != null) buttonNextStep.interactable =  currentPageIndex <= layout.steps.Length;

		if(buttonStartBuild != null) buttonStartBuild.gameObject.SetActive(currentPageIndex == 0);
		if(buttonValidateBuild != null) buttonValidateBuild.gameObject.SetActive(currentPageIndex == layout.steps.Length+1);

	}

	public void SetLayoutForGame(string filterType) {
		gameTypeFilter = filterType;
		
		filteredLayouts = layouts.FindAll(x => string.IsNullOrEmpty(filterType) || x.gameTypeFilter == filterType);

		//Debug.Log("SetLayoutForGame filterType("+filterType.ToString()+" filteredLayouts("+filteredLayouts.Count+")");

		currentLayoutIndex = 0;
		currentPageIndex = 0;
		currentPhase = BuildStepPhase.REQUESTING;
		Validated = false;

		RefreshPage();
	}

	public void NextLayout() {
		currentLayoutIndex++;
		if(currentLayoutIndex >= filteredLayouts.Count) currentLayoutIndex = 0;
		currentPageIndex = 0;
		currentPhase = BuildStepPhase.REQUESTING;
		Validated = false;
		RefreshPage();
	}
	
	public void PreviousLayout() {
		currentLayoutIndex--;
		if(currentLayoutIndex < 0) currentLayoutIndex = filteredLayouts.Count - 1;
		currentPageIndex = 0;
		currentPhase = BuildStepPhase.REQUESTING;
		Validated = false;
		RefreshPage();
	}

	public void NextStep() {
		currentPageIndex = Mathf.Clamp(currentPageIndex + 1, 0, filteredLayouts[currentLayoutIndex].steps.Length+1);
		currentPhase = BuildStepPhase.REQUESTING;
		RefreshPage();
	}

	public void PreviousStep() {
		currentPageIndex = Mathf.Clamp(currentPageIndex - 1, 0, filteredLayouts[currentLayoutIndex].steps.Length+1);
		currentPhase = BuildStepPhase.REQUESTING;
		RefreshPage();
	}

	public void ValidateBuild() {
		Validated = true;
	}

	private BuildStepPhase GetNextBuildStepPhase() {
		switch(currentPhase) {
			case BuildStepPhase.REQUESTING:

				if(robot.carryingObjectID != 0) {
					for(int i=0;i<robot.knownObjects.Count;i++) {
						if(robot.knownObjects[i].ID != robot.carryingObjectID) continue;

						if(robot.knownObjects[i].ObjectType == currentStep.cube.objectType) {
							screenMessage.ShowMessageForDuration("Cozmo has picked up the correct type of block!", 5f, Color.green);
							//if(currentStep.cube.propType == activeblock) return BuildStepPhase.CHANGING;
							//currentObjectType = robot.knownObjects[i].ObjectType;
							return BuildStepPhase.PLACING;
						}
						else {
							screenMessage.ShowMessageForDuration("Cozmo is holding the wrong type of block!", 5f, Color.red);
						}
						break;
					}
				}

				break;
			case BuildStepPhase.CHANGING:

				//cue player to change block state to requested color
				//if


				break;
			case BuildStepPhase.PLACING:
				if(iObjectPlaced) {
					//check if it is the right kind of object, and whether it was placed in the right location?

					//if first object placed, then is automatically in correct loc
					//if second placed, then do distance check
					//if third+ placed, then do 2d rel check from other blocks
					//if stacked, then check if on top of correct block

					return BuildStepPhase.PLACED;
				}
				break;
			case BuildStepPhase.PLACED:
				//screenMessage.ShowMessageForDuration("Cozmo has placed the block correctly!", 5f, Color.green);
				break;
		}

		return currentPhase;
	}

	private void EnterPhase() {
		switch(currentPhase) {
			case BuildStepPhase.REQUESTING:
				if(buttonNextStep != null) buttonNextStep.interactable = false;
				break;
			case BuildStepPhase.CHANGING:
				break;
			case BuildStepPhase.PLACING:
				break;
			case BuildStepPhase.PLACED:
				if(buttonNextStep != null) buttonNextStep.interactable = true;
				break;
		}
	}

	private void UpdatePhase() {



		switch(currentPhase) {
			case BuildStepPhase.REQUESTING:
				break;
			case BuildStepPhase.CHANGING:
				break;
			case BuildStepPhase.PLACING:
				break;
			case BuildStepPhase.PLACED:
				break;
		}
	}

	private void ExitPhase() {
		switch(currentPhase) {
			case BuildStepPhase.REQUESTING:
				break;
			case BuildStepPhase.CHANGING:
				break;
			case BuildStepPhase.PLACING:
				break;
			case BuildStepPhase.PLACED:
				break;
		}
	}

	private void ResetFramewiseInputs() {
//		iObjectPickedUp = false;
		iObjectPlaced = false;
//		iObjectSeen = false;
	}

	private void SuccessOrFailure(bool success, ActionCompleted action_type) {
		if(success) {
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
					iObjectPlaced = true;
					break;
			case ActionCompleted.PLACE_OBJECT_LOW:
			case ActionCompleted.PLACE_OBJECT_HIGH:
					iObjectPlaced = true;
					break;
			case ActionCompleted.PICK_AND_PLACE_INCOMPLETE:
					iObjectPlaced = false;
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
