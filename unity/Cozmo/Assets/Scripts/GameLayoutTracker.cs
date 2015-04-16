using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using Anki.Cozmo;

public class GameLayoutTracker : MonoBehaviour {

	[SerializeField] List<GameLayout> layouts;
	[SerializeField] Text title;
	[SerializeField] Text instructions;
	[SerializeField] Button buttonPrevLayout;
	[SerializeField] Button buttonNextLayout;
	[SerializeField] Button buttonStartBuild;
	[SerializeField] Button buttonValidateBuild;
	[SerializeField] ScreenMessage screenMessage = null;
	[SerializeField] float coplanarFudge = 0.2f;
	[SerializeField] float distanceFudge = 0.5f;

	public bool Validated { get; private set; }

	List<GameLayout> filteredLayouts;
	string gameTypeFilter = null;

	int currentLayoutIndex = 0;
	int currentPageIndex = 0;

	bool dirty = false;
	Robot robot = null;

	public static GameLayoutTracker instance = null;

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

		instance = this;

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

		//if(dirty) 
			RefreshLayout();
		dirty = false;
	}

	void OnDisable() {
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure -= SuccessOrFailure;

		if(instance == this) instance = null;
	}

	void RefreshLayout () {

		for(int i=0; i<filteredLayouts.Count; i++) {
			filteredLayouts[i].gameObject.SetActive(currentLayoutIndex == i);
		}

		GameLayout layout = currentLayout;

		title.text = layout.title;

		bool completed = false;
		int validCount = 0;

		if(currentPageIndex == 0) {
			ShowAllBlocks();
		}
		else {
			validCount = ValidateBlocks();
			completed = validCount == layout.blocks.Count;
		}

		if(instructions != null) {
			if(currentPageIndex <= 0) {
				instructions.text = "Press to start building this game's layout!";
			}
			else if(completed) {
				instructions.text = "Layout completed, Tap here to start playing!";
			}
			else if(validCount == 0) {
				instructions.text = "Place blocks to set up this game!" ;
			}
			else {
				instructions.text = "Cozmo's build progress: " + validCount + " / " + layout.blocks.Count;
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

	void SuccessOrFailure(bool success, ActionCompleted action_type) {
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

	void ShowAllBlocks() {
		GameLayout layout = currentLayout;
		if(layout == null) return;

		for(int i=0; i<layout.blocks.Count; i++) {
			layout.blocks[i].Hidden = false;
			layout.blocks[i].Highlighted = false;
			layout.blocks[i].Validated = true;
			layout.blocks[i].AssignedObjectID = -1;
		}
	}

	int ValidateBlocks() {
		GameLayout layout = currentLayout;
		if(layout == null) return 0;

		List<BuildInstructionsCube> validated = new List<BuildInstructionsCube>();

		for(int layoutBlockIndex=0; layoutBlockIndex<layout.blocks.Count; layoutBlockIndex++) {
			BuildInstructionsCube block = currentLayout.blocks[layoutBlockIndex];

			block.Hidden = false;
			block.Highlighted = true;
			block.Validated = false;
			block.AssignedObjectID = -1;

			//Debug.Log("attempting to validate block of type("+block.objectType+")");

			//cannot validate a block before the one it is stacked upon
			if(block.cubeBelow != null && !block.cubeBelow.Validated) continue;

			//search through known objects for one that can be assigned
			for(int knownObjectIndex=0; knownObjectIndex<robot.knownObjects.Count && validated.Count < layout.blocks.Count; knownObjectIndex++) {

				ObservedObject newObject = robot.knownObjects[knownObjectIndex];

				//skip objects of the wrong type
				if(block.objectType != (int)newObject.ObjectType) continue;

				//skip objects already assigned to a layout block
				if(layout.blocks.Find( x => x.AssignedObjectID == newObject.ID) != null) continue;

				if(validated.Count == 0) {
					validated.Add(block);
					block.AssignedObjectID = newObject.ID;
					block.Validated = true;
					block.Highlighted = false;
				}
				//if this ideal block needs to get stacked on one we already know about...
				else if(block.cubeBelow != null) {

					ObservedObject objectToStackUpon = robot.knownObjects.Find( x => x.ID == block.cubeBelow.AssignedObjectID);

					Vector3 real = (newObject.WorldPosition - objectToStackUpon.WorldPosition) / CozmoUtil.BLOCK_LENGTH_MM;
					float dist = ((Vector2)real).magnitude;
					bool valid = dist < distanceFudge && Mathf.Abs(1f - real.z) < coplanarFudge;

					if(valid) {
						validated.Add(block);
						block.AssignedObjectID = newObject.ID;
						block.Validated = true;
						block.Highlighted = false;
					}
					else {
						Debug.Log("stack test failed for blockType("+block.objectType+") on blockType("+block.cubeBelow.objectType+") dist("+dist+") real.z("+real.z+")");
					}
				}
				//if we have some blocks validated so far, then we'll do distance checks to each
				// and also confirm relative angles?
				else {


					bool valid = true;

					for(int validatedIndex=0;validatedIndex < validated.Count;validatedIndex++) {
						//let's just compared distances from unstacked blocks
						if(validated[validatedIndex].cubeBelow != null) continue;

						Vector3 idealOffset = (block.transform.position - validated[validatedIndex].transform.position) / block.Size;
						float up = idealOffset.y;
						float forward = idealOffset.z;
						idealOffset.y = forward;
						idealOffset.z = up;
						
						ObservedObject priorObject = robot.knownObjects.Find( x => x.ID == validated[validatedIndex].AssignedObjectID);
						Vector3 realOffset = (newObject.WorldPosition - priorObject.WorldPosition) / CozmoUtil.BLOCK_LENGTH_MM;
						
						//are we basically on the same plane and roughly the correct distance away?
						if(Mathf.Abs(realOffset.z) > coplanarFudge){
							valid = false;
							Debug.Log("zOffset("+realOffset.z+") invalidated that block of type("+block.objectType+") is on same plane as previously validated block of type("+validated[validatedIndex].objectType+")");
							break;
						}

						float idealDistance = ((Vector2)idealOffset).magnitude;
						float realDistance = ((Vector2)realOffset).magnitude;

						float distanceError = Mathf.Abs(realDistance - idealDistance);
						if( distanceError > distanceFudge ) {
							valid = false;
							Debug.Log("error("+distanceError+") invalidated that block of type("+block.objectType+") is the correct distance from previously validated block of type("+validated[validatedIndex].objectType+") idealDistance("+idealDistance+") realDistance("+realDistance+")");
							break;
						}

					}

					if(valid) {
						validated.Add(block);
						block.AssignedObjectID = newObject.ID;
						block.Validated = true;
						block.Highlighted = false;
					}
				}

			}

		}

		return validated.Count;
	}

}
