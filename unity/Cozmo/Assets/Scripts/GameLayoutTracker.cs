using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using Anki.Cozmo;

public class GameLayoutTracker : MonoBehaviour {

	[SerializeField] GameObject ghostPrefab = null;
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
	bool showPreview = true;

	bool dirty = false;
	Robot robot = null;

	bool blocksInitialized = false;

	BuildInstructionsCube ghostBlock;

	public static GameLayoutTracker instance = null;

	GameLayout currentLayout {
		get {
			if(filteredLayouts == null) return null;
			if(currentLayoutIndex < 0) return null;
			if(currentLayoutIndex >= filteredLayouts.Count) return null;
			return filteredLayouts[currentLayoutIndex];
		}
	}

	void OnEnable () {

		if(!blocksInitialized) {
			for(int i=0; i<layouts.Count; i++) {
				layouts[i].Initialize();
			}
			blocksInitialized = true;
		}

		instance = this;

		currentLayoutIndex = 0;
		showPreview = true;

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

		//if this build is underway, let's skip the preview page
		int validCount = ValidateBlocks();
		if(validCount > 0) showPreview = false;

		if(ghostBlock == null) {
			GameObject ghost = (GameObject)GameObject.Instantiate(ghostPrefab);
			ghostBlock = ghost.GetComponent<BuildInstructionsCube>();
		}

		if(ghostBlock != null) {
			ghostBlock.Initialize();
			ghostBlock.Hidden = true;
			ghostBlock.Highlighted = true;
			ghostBlock.gameObject.SetActive(false);
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

		if(showPreview) {
			ShowAllBlocks();
		}
		else {
			validCount = ValidateBlocks();
			completed = validCount == layout.blocks.Count;
		}

		if(instructions != null) {
			if(showPreview) {
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

		if(buttonStartBuild != null) buttonStartBuild.gameObject.SetActive(showPreview);
		if(buttonValidateBuild != null) buttonValidateBuild.gameObject.SetActive(completed);


		Validated = completed;
	}

	public void SetLayoutForGame(string filterType) {
		gameTypeFilter = filterType;
		
		filteredLayouts = layouts.FindAll(x => string.IsNullOrEmpty(filterType) || x.gameTypeFilter == filterType);

		//Debug.Log("SetLayoutForGame filterType("+filterType.ToString()+" filteredLayouts("+filteredLayouts.Count+")");

		currentLayoutIndex = 0;
		showPreview = true;
		Validated = false;

		RefreshLayout();
	}

	public void NextLayout() {
		currentLayoutIndex++;
		if(currentLayoutIndex >= filteredLayouts.Count) currentLayoutIndex = 0;
		showPreview = true;
		Validated = false;
		RefreshLayout();
	}
	
	public void PreviousLayout() {
		currentLayoutIndex--;
		if(currentLayoutIndex < 0) currentLayoutIndex = filteredLayouts.Count - 1;
		showPreview = true;
		Validated = false;
		RefreshLayout();
	}

	public void StartBuildingLayout() {
		showPreview = false;
		RefreshLayout();
	}
	
	public void ValidateLayoutAndStartGame() {
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
		if(ghostBlock != null) ghostBlock.gameObject.SetActive(false);

		GameLayout layout = currentLayout;
		if(layout == null) return 0;

		List<BuildInstructionsCube> validated = new List<BuildInstructionsCube>();

		//loop through our 'ideal' layout blocks and look for known objects that might satisfy the requirements of each
		for(int layoutBlockIndex=0; layoutBlockIndex<layout.blocks.Count; layoutBlockIndex++) {
			BuildInstructionsCube block = currentLayout.blocks[layoutBlockIndex];

			block.Hidden = false;
			block.Highlighted = true;
			block.Validated = false;
			block.AssignedObjectID = -1;

			//Debug.Log("attempting to validate block of type("+block.objectType+")");

			//cannot validate a block before the one it is stacked upon
			//note: this requires that our ideal blocks be listed in order
			//		which is necessary to get the highlight lines to draw correctly anyhow
			if(block.cubeBelow != null && !block.cubeBelow.Validated) continue;

			//search through known objects for one that can be assigned
			for(int knownObjectIndex=0; knownObjectIndex<robot.knownObjects.Count && validated.Count < layout.blocks.Count; knownObjectIndex++) {

				ObservedObject newObject = robot.knownObjects[knownObjectIndex];

				//skip objects of the wrong type
				if(block.objectType != (int)newObject.ObjectType) continue;
				if(block.objectFamily != (int)newObject.Family) continue;

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
						PlaceGhostForObservedObject(newObject, block, objectToStackUpon, block.cubeBelow);
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
							PlaceGhostForObservedObject(newObject, block, priorObject, validated[validatedIndex]);
							Debug.Log("zOffset("+realOffset.z+") invalidated that block of type("+block.objectType+") is on same plane as previously validated block of type("+validated[validatedIndex].objectType+")");
							break;
						}

						float idealDistance = ((Vector2)idealOffset).magnitude;
						float realDistance = ((Vector2)realOffset).magnitude;

						float distanceError = Mathf.Abs(realDistance - idealDistance);
						if( distanceError > distanceFudge ) {
							valid = false;
							PlaceGhostForObservedObject(newObject, block, priorObject, validated[validatedIndex]);
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

	//place our ghost block in the layout window at the position of the currently failing
	void PlaceGhostForObservedObject(ObservedObject failingObject, BuildInstructionsCube failingLayoutBlock, ObservedObject validObject, BuildInstructionsCube validLayoutBlock) {
		if(ghostBlock == null) return;
		//just use the ghost for the first fail detected
		if(ghostBlock.gameObject.activeSelf) return;

		Vector3 objectOffset = (failingObject.WorldPosition - validObject.WorldPosition) / CozmoUtil.BLOCK_LENGTH_MM;
		//convert to unity space
		objectOffset = new Vector3(objectOffset.x, objectOffset.z, objectOffset.y) * validLayoutBlock.Size;

		//if we failed a distance check, let's place the ghost along the relevant line
		if(failingLayoutBlock.cubeBelow == null) {
			Vector3 flatOffset = objectOffset;
			flatOffset.y = 0;
			float flatMag = flatOffset.magnitude;
			flatOffset = failingLayoutBlock.transform.position - validLayoutBlock.transform.position;
			flatOffset.y = 0;
			flatOffset = flatOffset.normalized * flatMag;
			objectOffset.x = flatOffset.x;
			objectOffset.z = flatOffset.z;
		}

		ghostBlock.transform.position = validLayoutBlock.transform.position + objectOffset;
		ghostBlock.baseColor = failingLayoutBlock.baseColor;
		ghostBlock.Highlighted = true;
		ghostBlock.gameObject.SetActive(true);
		Debug.Log("ghostBlock placed!");
	}

}
