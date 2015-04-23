using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using Anki.Cozmo;
using G2U = Anki.Cozmo.G2U;
using U2G = Anki.Cozmo.U2G;

public class GameLayoutTracker : MonoBehaviour {

	public static GameLayoutTracker instance = null;

	public bool Validated { get; private set; }

	public enum LayoutTrackerPhase {
		DISABLED,
		PREVIEW,
		BUILDING,
		COMPLETE
	}

	[SerializeField] bool debug = false;
	[SerializeField] List<GameLayout> layouts;

	[SerializeField] GameObject layoutPreviewPanel = null;
	[SerializeField] GameObject hideDuringPreview = null;
	[SerializeField] Text previewTitle;

	[SerializeField] GameObject layoutInstructionsPanel = null;
	[SerializeField] Camera layoutInstructionsCamera = null;
	[SerializeField] GameObject ghostPrefab = null;
	[SerializeField] Text instructionsTitle;
	[SerializeField] Text instructionsProgress;
	[SerializeField] Button buttonStartPlaying;
	[SerializeField] ScreenMessage screenMessage = null;
	[SerializeField] float coplanarFudge = 0.2f;
	[SerializeField] float distanceFudge = 0.5f;



	Robot robot = null;
	bool blocksInitialized = false;
	BuildInstructionsCube ghostBlock;

	LayoutTrackerPhase _phase = LayoutTrackerPhase.DISABLED;
	public LayoutTrackerPhase Phase {
		get {
			return _phase;
		}

		private set {
			_phase = value;
		}
	}

	GameLayout currentLayout = null;
	string currentGameName = "Unknown";
	int currentLevelNumber = 1;
	
	int validCount = 0;

	void OnEnable () {
		Phase = LayoutTrackerPhase.PREVIEW;

		if(!blocksInitialized) {
			for(int i=0; i<layouts.Count; i++) {
				layouts[i].Initialize();
			}
			blocksInitialized = true;
		}

		instance = this;

		Validated = false;
		validCount = 0;

		currentGameName = PlayerPrefs.GetString("CurrentGame", "Slalom");
		currentLevelNumber = PlayerPrefs.GetInt(currentGameName + "_CurrentLevel", 1);

		currentLayout = null;

		for(int i=0; i<layouts.Count; i++) {
			layouts[i].gameObject.SetActive(layouts[i].gameName == currentGameName && layouts[i].levelNumber == currentLevelNumber);

			if(layouts[i].gameObject.activeSelf) {
				currentLayout = layouts[i];
			}
		}

		layoutInstructionsPanel.SetActive(false);

		//no apt layout found?  then just disable
		if(currentLayout == null) {
			layoutPreviewPanel.SetActive(false);
			hideDuringPreview.SetActive(true);
			Phase = LayoutTrackerPhase.DISABLED;
			return;
		}

		currentLayout.previewCamera.gameObject.SetActive(true);
		string fullName = currentGameName + " #" + currentLevelNumber;
		previewTitle.text = fullName;
		instructionsTitle.text = fullName;

		RefreshLayout();

		if(ghostBlock == null) {
			GameObject ghost = (GameObject)GameObject.Instantiate(ghostPrefab);
			ghostBlock = ghost.GetComponent<BuildInstructionsCube>();
		}

		if(ghostBlock != null) {
			ghostBlock.gameObject.SetActive(true);
			ghostBlock.Initialize();
			ghostBlock.Hidden = true;
			ghostBlock.Highlighted = false;
		}

		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure += SuccessOrFailure;
	}

	void Update () {
		RefreshLayout();
	}

	void OnDisable() {
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure -= SuccessOrFailure;

		if(instance == this) instance = null;

		if(ghostBlock != null) {
			ghostBlock.gameObject.SetActive(false);
		}

		if(hideDuringPreview != null) hideDuringPreview.SetActive(true);
	}

	void RefreshLayout () {

		bool completed = validCount == currentLayout.blocks.Count;

		if(Phase == LayoutTrackerPhase.BUILDING && completed) {
			Phase = LayoutTrackerPhase.COMPLETE;
		}

		currentLayout.previewCamera.gameObject.SetActive(Phase == LayoutTrackerPhase.PREVIEW);

		switch(Phase) {
			case LayoutTrackerPhase.DISABLED:
				layoutInstructionsPanel.SetActive(false);
				layoutPreviewPanel.SetActive(false);
				hideDuringPreview.SetActive(true);
				layoutInstructionsCamera.gameObject.SetActive(false);
				break;

			case LayoutTrackerPhase.PREVIEW:
				ShowAllBlocks();
				layoutInstructionsPanel.SetActive(false);
				layoutPreviewPanel.SetActive(true);
				hideDuringPreview.SetActive(false);
				layoutInstructionsCamera.gameObject.SetActive(false);
				break;

			case LayoutTrackerPhase.BUILDING:
				instructionsProgress.text = "Cozmo's build progress: " + validCount + " / " + currentLayout.blocks.Count;
				layoutInstructionsPanel.SetActive(true);
				layoutPreviewPanel.SetActive(false);
				hideDuringPreview.SetActive(true);
				layoutInstructionsCamera.gameObject.SetActive(true);
				break;

			case LayoutTrackerPhase.COMPLETE:
				instructionsProgress.text = "Layout completed!";
				layoutInstructionsPanel.SetActive(true);
				layoutPreviewPanel.SetActive(false);
				hideDuringPreview.SetActive(true);
				layoutInstructionsCamera.gameObject.SetActive(true);
				break;

		}

		buttonStartPlaying.gameObject.SetActive(Phase == LayoutTrackerPhase.COMPLETE);
	}

	void SuccessOrFailure(bool success, ActionCompleted action_type) {
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

		validCount = ValidateBlocks();
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
		if(RobotEngineManager.instance == null) return 0;

		robot = RobotEngineManager.instance.current;
		if(robot == null) return 0;

		if(ghostBlock != null) {
			ghostBlock.Hidden = true;
			ghostBlock.Highlighted = false;
		}

		GameLayout layout = currentLayout;
		if(layout == null) return 0;

		List<BuildInstructionsCube> validated = new List<BuildInstructionsCube>();

		//first loop through and clear our old assignments
		for(int layoutBlockIndex=0; layoutBlockIndex<layout.blocks.Count; layoutBlockIndex++) {
			BuildInstructionsCube block = layout.blocks[layoutBlockIndex];
			block.Hidden = false;
			block.Highlighted = true;
			block.Validated = false;
			block.AssignedObjectID = -1;
		}

		//loop through our 'ideal' layout blocks and look for known objects that might satisfy the requirements of each
		for(int layoutBlockIndex=0; layoutBlockIndex<layout.blocks.Count; layoutBlockIndex++) {
			BuildInstructionsCube block = layout.blocks[layoutBlockIndex];

			if(debug) Debug.Log("attempting to validate block("+block.gameObject.name+") of type("+block.objectType+")");

			//cannot validate a block before the one it is stacked upon
			//note: this requires that our ideal blocks be listed in order
			//		which is necessary to get the highlight lines to draw correctly anyhow
			if(block.cubeBelow != null && !block.cubeBelow.Validated) continue;

			//search through known objects for one that can be assigned
			for(int knownObjectIndex=0; knownObjectIndex<robot.knownObjects.Count && validated.Count < layout.blocks.Count; knownObjectIndex++) {

				ObservedObject newObject = robot.knownObjects[knownObjectIndex];

				if(block.objectFamily != (int)newObject.Family) {
					continue;
				}

				//skip objects of the wrong type
				if(block.objectFamily != 3 && block.objectType != (int)newObject.ObjectType) {
					continue;
				}

				if(block.objectFamily == 3 && block.activeBlockType != newObject.activeBlockType) { //active block
					continue;
				}

				//skip objects already assigned to a layout block
				if(layout.blocks.Find( x => x.AssignedObjectID == newObject) != null) continue;

				if(validated.Count == 0) {
					validated.Add(block);
					block.AssignedObjectID = newObject;
					block.Validated = true;
					block.Highlighted = false;
					if(debug) Debug.Log("validated block("+block.gameObject.name+") of type("+block.objectType+") family("+block.objectFamily+") because first block placed on ground.");
				}
				//if this ideal block needs to get stacked on one we already know about...
				else if(block.cubeBelow != null) {

					ObservedObject objectToStackUpon = robot.knownObjects.Find( x => x == block.cubeBelow.AssignedObjectID);

					Vector3 real = (newObject.WorldPosition - objectToStackUpon.WorldPosition) / CozmoUtil.BLOCK_LENGTH_MM;
					float dist = ((Vector2)real).magnitude;
					bool valid = dist < distanceFudge && Mathf.Abs(1f - real.z) < coplanarFudge;

					if(valid) {
						validated.Add(block);
						block.AssignedObjectID = newObject;
						block.Validated = true;
						block.Highlighted = false;
						if(debug) Debug.Log("validated block("+block.gameObject.name+") of type("+block.objectType+") family("+block.objectFamily+") because stacked on apt block.");
					}
					else {
						if(debug) Debug.Log("stack test failed for blockType("+block.objectType+") on blockType("+block.cubeBelow.objectType+") dist("+dist+") real.z("+real.z+")");
						PlaceGhostForObservedObject(newObject, block, objectToStackUpon, block.cubeBelow);
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
						
						ObservedObject priorObject = robot.knownObjects.Find( x => x == validated[validatedIndex].AssignedObjectID);
						Vector3 realOffset = (newObject.WorldPosition - priorObject.WorldPosition) / CozmoUtil.BLOCK_LENGTH_MM;
						
						//are we basically on the same plane and roughly the correct distance away?
						if(Mathf.Abs(realOffset.z) > coplanarFudge){
							valid = false;
							if(debug) Debug.Log("zOffset("+realOffset.z+") invalidated that block of type("+block.objectType+") is on same plane as previously validated block of type("+validated[validatedIndex].objectType+")");
							PlaceGhostForObservedObject(newObject, block, priorObject, validated[validatedIndex]);
							break;
						}

						float idealDistance = ((Vector2)idealOffset).magnitude;
						float realDistance = ((Vector2)realOffset).magnitude;

						float distanceError = Mathf.Abs(realDistance - idealDistance);
						if( distanceError > distanceFudge ) {
							valid = false;
							if(debug) Debug.Log("error("+distanceError+") invalidated that block of type("+block.objectType+") is the correct distance from previously validated block of type("+validated[validatedIndex].objectType+") idealDistance("+idealDistance+") realDistance("+realDistance+")");
							PlaceGhostForObservedObject(newObject, block, priorObject, validated[validatedIndex]);
							break;
						}

					}

					if(valid) {
						validated.Add(block);
						block.AssignedObjectID = newObject;
						block.Validated = true;
						block.Highlighted = false;
						if(debug) Debug.Log("validated block("+block.gameObject.name+") of type("+block.objectType+") family("+block.objectFamily+") because correct distance on ground from valid blocks.");
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
		if(ghostBlock.Highlighted) return;

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

		//ghostBlock.gameObject.SetActive(true);
		ghostBlock.transform.position = validLayoutBlock.transform.position + objectOffset;
		ghostBlock.objectType = failingLayoutBlock.objectType;
		ghostBlock.objectFamily = failingLayoutBlock.objectFamily;
		ghostBlock.activeBlockType = failingLayoutBlock.activeBlockType;
		ghostBlock.baseColor = failingLayoutBlock.baseColor;
		ghostBlock.Hidden = true;
		ghostBlock.Highlighted = true;

		//Debug.Log("ghostBlock type("+ghostBlock.objectType+") family("+ghostBlock.objectFamily+") baseColor("+ghostBlock.baseColor+")");
	}

	public void ValidateBuild() {
		Validated = true;
		Phase = LayoutTrackerPhase.DISABLED;
	}

	public void EndPreview() {
		Phase = LayoutTrackerPhase.BUILDING;
		validCount = ValidateBlocks();
	}

}
