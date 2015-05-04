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
		INVENTORY,
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

	[SerializeField] GameObject block2dPrefab = null;
	[SerializeField] RectTransform block2dAnchor = null;

	[SerializeField] Text instructionsTitle;
	[SerializeField] Text instructionsProgress;
	[SerializeField] Button buttonStartPlaying;
	[SerializeField] ScreenMessage screenMessage = null;
	[SerializeField] float coplanarFudge = 0.5f;
	[SerializeField] float distanceFudge = 0.5f;

	[SerializeField] AudioClip blockValidatedSound = null;
	[SerializeField] AudioClip layoutValidatedSound = null;
	
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
	int lastValidCount = 0;

	bool hidden = false;

	List<LayoutBlock2d> blocks2d = new List<LayoutBlock2d>();

	void OnEnable () {
		Phase = LayoutTrackerPhase.INVENTORY;

		if(!blocksInitialized) {
			for(int i=0; i<layouts.Count; i++) {
				layouts[i].Initialize();
			}
			blocksInitialized = true;
		}

		instance = this;

		Validated = false;
		validCount = 0;
		lastValidCount = 0;

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

		SetUpInventory();

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

		hidden = false;
	}

	void SetUpInventory() {
	
		blocks2d.Clear();
		
		int count = currentLayout.blocks.Count;
		for(int i=0; i<count; i++) {
			BuildInstructionsCube block = currentLayout.blocks[i];
			GameObject block2dObj = (GameObject)GameObject.Instantiate(block2dPrefab);
			RectTransform blockTrans = block2dObj.transform as RectTransform;
			blockTrans.SetParent(block2dAnchor, false);
			Vector2 pos = blockTrans.anchoredPosition;
			float w = blockTrans.sizeDelta.x * 1.5f;
			pos.x = (-w * count * 0.5f) + w * (i + 0.5f);
			
			blockTrans.anchoredPosition = pos;
			LayoutBlock2d block2d = block2dObj.GetComponent<LayoutBlock2d>();

			//count any duplicates so this one knows when to be validated
			int num = 1;
			foreach(LayoutBlock2d prior in blocks2d) {
				if(prior.Block.objectFamily != block.objectFamily) continue;
				if(prior.Block.objectFamily != 3 && prior.Block.objectType != block.objectType) continue;
				if(prior.Block.objectFamily == 3 && prior.Block.activeBlockType != block.activeBlockType) continue;
				num++;
			}
			
			block2d.Initialize(block, num);
			
			blocks2d.Add(block2d);
		}
	}

	void Update () {

		if(Input.GetKeyDown(KeyCode.V)) {
			ValidateBlocks();
		}

		if(validCount == currentLayout.blocks.Count) {
			if(lastValidCount != currentLayout.blocks.Count) {
				audio.PlayOneShot(layoutValidatedSound);
			}
		}
		else if(validCount > lastValidCount) {
			audio.PlayOneShot(blockValidatedSound);
		}


		RefreshLayout();

		lastValidCount = validCount;
	}

	void OnDisable() {
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure -= SuccessOrFailure;

		if(instance == this) instance = null;

		if(ghostBlock != null) {
			ghostBlock.gameObject.SetActive(false);
		}

		if(hideDuringPreview != null) hideDuringPreview.SetActive(true);
	}

	public void Show() {
		hidden = false;
	}

	public void Hide() {
		hidden = true;
		layoutInstructionsPanel.SetActive(false);
		layoutPreviewPanel.SetActive(false);
		layoutInstructionsCamera.gameObject.SetActive(false);
	}

	void RefreshLayout () {

		bool completed = validCount == currentLayout.blocks.Count;

		if(Phase == LayoutTrackerPhase.BUILDING && completed) {
			Phase = LayoutTrackerPhase.COMPLETE;
		}

		robot = RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null;

		currentLayout.previewCamera.gameObject.SetActive(Phase == LayoutTrackerPhase.INVENTORY);

		switch(Phase) {
			case LayoutTrackerPhase.DISABLED:
				layoutInstructionsPanel.SetActive(false);
				layoutPreviewPanel.SetActive(false);
				hideDuringPreview.SetActive(true);
				layoutInstructionsCamera.gameObject.SetActive(false);
				break;

			case LayoutTrackerPhase.INVENTORY:
				ShowAllBlocks();
				layoutInstructionsPanel.SetActive(false);
				layoutPreviewPanel.SetActive(!hidden);
				hideDuringPreview.SetActive(false);
				layoutInstructionsCamera.gameObject.SetActive(false);

				if(robot != null) robot.SetHeadAngle();

				for(int i=0;i<blocks2d.Count;i++) {
					int known = GetKnownObjectCountForBlock(blocks2d[i].Block);
					blocks2d[i].Validate(known >= blocks2d[i].Dupe);
				}

				break;

			case LayoutTrackerPhase.BUILDING:
				if(validCount == 0 && !string.IsNullOrEmpty(currentLayout.initialInstruction)) {
					instructionsProgress.text = currentLayout.initialInstruction;
				}
				else if(validCount == 1 && !string.IsNullOrEmpty(currentLayout.secondInstruction)) {
					instructionsProgress.text = currentLayout.secondInstruction;
				}
				else {
					instructionsProgress.text = "Cozmo's build progress: " + validCount + " / " + currentLayout.blocks.Count;
				}

				layoutInstructionsPanel.SetActive(!hidden);
				layoutPreviewPanel.SetActive(false);
				hideDuringPreview.SetActive(true);
				layoutInstructionsCamera.gameObject.SetActive(!hidden);
				break;

			case LayoutTrackerPhase.COMPLETE:
				instructionsProgress.text = "Layout completed!";
				layoutInstructionsPanel.SetActive(!hidden);
				layoutPreviewPanel.SetActive(false);
				hideDuringPreview.SetActive(true);
				layoutInstructionsCamera.gameObject.SetActive(!hidden);

				break;

		}

		buttonStartPlaying.gameObject.SetActive(Phase == LayoutTrackerPhase.COMPLETE);
	}

	void SuccessOrFailure(bool success, RobotActionType action_type) {
		if(Phase != LayoutTrackerPhase.BUILDING) return;

		bool validate = false;

		switch(action_type) {
			case RobotActionType.UNKNOWN://	ACTION_UNKNOWN = -1,
				break;
			case RobotActionType.DRIVE_TO_POSE:
				break;
			case RobotActionType.DRIVE_TO_OBJECT:
				break;
			case RobotActionType.DRIVE_TO_PLACE_CARRIED_OBJECT:
				break;
			case RobotActionType.TURN_IN_PLACE:
				break;
			case RobotActionType.MOVE_HEAD_TO_ANGLE:
				break;
			case RobotActionType.PICKUP_OBJECT_LOW:
			case RobotActionType.PICKUP_OBJECT_HIGH:
				BuildInstructionsCube layoutCubeMatchingCarried = null;
				if(robot.carryingObject >= 0) {
					for(int i=0;i<currentLayout.blocks.Count;i++) {
						if(robot.carryingObject.Family == 3 && currentLayout.blocks[i].objectFamily != 3) continue;
						if(robot.carryingObject.Family != 3 && currentLayout.blocks[i].objectType != (int)robot.carryingObject.ObjectType) continue;
						layoutCubeMatchingCarried = currentLayout.blocks[i];
						break;
					}
					if(layoutCubeMatchingCarried.objectFamily == 3 && robot.carryingObject.activeBlockType != layoutCubeMatchingCarried.activeBlockType) {
						screenMessage.ShowMessage("Cozmo can now CHANGE this block's color to "+layoutCubeMatchingCarried.activeBlockType.ToString()+".", Color.white);
					}
					else if(layoutCubeMatchingCarried.cubeBelow == null) {
						if(validCount == 0) {
							screenMessage.ShowMessage("Good.  Now drop this block anywhere to start the build.", Color.white);
						}
						else {
							screenMessage.ShowMessage("Good.  Now drop this block the correct distance from the first block.", Color.white);
						}
					}
					else {
						screenMessage.ShowMessage("Good.  Now stack this block on a correct block.", Color.white);
					}
				}
				else {
					screenMessage.KillMessage();
				}
				
				break;
			case RobotActionType.PLACE_OBJECT_LOW:
			case RobotActionType.PLACE_OBJECT_HIGH:
				validate = true;
				break;
			case RobotActionType.CROSS_BRIDGE:
			case RobotActionType.ASCEND_OR_DESCEND_RAMP:
			case RobotActionType.TRAVERSE_OBJECT:
			case RobotActionType.DRIVE_TO_AND_TRAVERSE_OBJECT:
			case RobotActionType.FACE_OBJECT:
			case RobotActionType.PLAY_ANIMATION:
			case RobotActionType.PLAY_SOUND:
			case RobotActionType.WAIT:
				break;
		}

		if(!success) {
			screenMessage.ShowMessageForDuration("Cozmo ran into difficulty, let's try that again.", 5f, Color.yellow);
		}

		if(validate) {
			string error = ValidateBlocks();
			if(validCount <= lastValidCount) {
				screenMessage.ShowMessage(error, Color.red);
			}
			else {
				//Debug.Log("validation sucess.  lastValidCount("+lastValidCount+")->validCount("+validCount+")");
				screenMessage.ShowMessage("Awesome!  Cozmo placed that block correctly.", Color.white);
			}
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

	List<BuildInstructionsCube> validated = new List<BuildInstructionsCube>();
	string ValidateBlocks() {
		string error = "";
		validCount = 0;

		if(RobotEngineManager.instance == null) return error;

		robot = RobotEngineManager.instance.current;
		if(robot == null) return error;

		bool invalidated = false;

		if(ghostBlock != null) {
			ghostBlock.Hidden = true;
			ghostBlock.Highlighted = false;
		}

		GameLayout layout = currentLayout;
		if(layout == null) return error;

		validated.Clear();

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
					if(debug) Debug.Log("skip objects of the wrong type");
					continue;
				}

				if(block.objectFamily == 3 && block.activeBlockType != newObject.activeBlockType) { //active block
					if(debug) Debug.Log("skip active block of the wrong color. goalColor("+block.activeBlockType+") newObject("+newObject+"):color("+newObject.activeBlockType+")");
					continue;
				}

				//skip objects already assigned to a layout block
				if(layout.blocks.Find( x => x.AssignedObjectID == newObject) != null) continue;

				if(validated.Count == 0) {
					ValidateBlock(layoutBlockIndex, newObject);
					if(debug) Debug.Log("validated block("+block.gameObject.name+") of type("+block.objectType+") family("+block.objectFamily+") because first block placed on ground.");
				}
				//if this ideal block needs to get stacked on one we already know about...
				else if(block.cubeBelow != null) {

					ObservedObject objectToStackUpon = robot.knownObjects.Find( x => x == block.cubeBelow.AssignedObjectID);

					Vector3 real = (newObject.WorldPosition - objectToStackUpon.WorldPosition) / CozmoUtil.BLOCK_LENGTH_MM;
					float dist = ((Vector2)real).magnitude;
					bool valid = dist < distanceFudge && Mathf.Abs(1f - real.z) < coplanarFudge;

					if(valid) {
						ValidateBlock(layoutBlockIndex, newObject);
						if(debug) Debug.Log("validated block("+block.gameObject.name+") of type("+block.objectType+") family("+block.objectFamily+") because stacked on apt block.");
					}
					else {
						if(!invalidated) error = "Whoops. That block needs to be stacked on the correct block.";
						invalidated = true;
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
						if(Mathf.Abs(realOffset.z) > coplanarFudge) {
							if(!invalidated) error = "Whoops. That block needs to be placed on the ground.";
							invalidated = true;
							valid = false;
							if(debug) Debug.Log("zOffset("+realOffset.z+") invalidated that block of type("+block.objectType+") is on same plane as previously validated block of type("+validated[validatedIndex].objectType+")");
							PlaceGhostForObservedObject(newObject, block, priorObject, validated[validatedIndex]);
							break;
						}

						float idealDistance = ((Vector2)idealOffset).magnitude;
						float realDistance = ((Vector2)realOffset).magnitude;

						float distanceError = Mathf.Abs(realDistance - idealDistance);
						if( distanceError > distanceFudge ) {
							if(!invalidated) error = "Whoops. That block was placed " + distanceError.ToString("N") + " block lengths too " + (realDistance > idealDistance ? "far from" : "close to" ) + " the " + CozmoPalette.instance.GetNameForObjectType(validated[validatedIndex].objectType) + " block.";
							invalidated = true;
							valid = false;
							if(debug) Debug.Log("error("+distanceError+") invalidated that block of type("+block.objectType+") is the correct distance from previously validated block of type("+validated[validatedIndex].objectType+") idealDistance("+idealDistance+") realDistance("+realDistance+")");
							PlaceGhostForObservedObject(newObject, block, priorObject, validated[validatedIndex]);
							break;
						}

					}

					if(valid) {
						ValidateBlock(layoutBlockIndex, newObject);
						if(debug) Debug.Log("validated block("+block.gameObject.name+") of type("+block.objectType+") family("+block.objectFamily+") because correct distance on ground from valid blocks.");
					}
				}

			}

		}

		validCount = validated.Count;

		return error;
	}

	void ValidateBlock(int layoutBlockIndex, int newObject) {
		BuildInstructionsCube block = currentLayout.blocks[layoutBlockIndex];
		validated.Add(block);
		block.AssignedObjectID = newObject;
		block.Validated = true;
		block.Highlighted = false;
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

	int GetKnownObjectCountForBlock(BuildInstructionsCube block) {
		if(robot == null) return 0;

		int count = 0;

		//Debug.Log("GetKnownObjectCountForBlock robot.knownObjects.Count("+robot.knownObjects.Count+")");

		for(int i=0;i<robot.knownObjects.Count;i++) {

			ObservedObject obj = robot.knownObjects[i];

			if(obj.Family != block.objectFamily) continue;
			if(obj.Family != 3 && obj.ObjectType != block.objectType) continue;
			//if(obj.Family == 3 && obj.activeBlockType != block.activeBlockType) continue;

			count++;
		}

		return count;
	}

	public void ValidateBuild() {
		Validated = true;
		Phase = LayoutTrackerPhase.DISABLED;
	}

	public void EndPreview() {
		Phase = LayoutTrackerPhase.BUILDING;
		ValidateBlocks();
	}

	public Vector3 GetStartingPositionFromLayout() {
		if(currentLayout == null) return Vector3.zero;
		if(currentLayout.startPositionMarker == null) return Vector3.zero;
	
		//ObservedObject firstObj = null;
		//ObservedObject secondObj = null;
		//Vector3 layoutRight = currentLayout.blocks[1].AssignedObjectID


		Vector3 pos = currentLayout.startPositionMarker.position;
		float forward = pos.z;
		float up = pos.y;
		pos.y = forward;
		pos.z = up;

		return pos;
	}

	public float GetStartingAngleFromLayout() {
		if(currentLayout == null) return 0f;
		if(currentLayout.startPositionMarker == null) return 0f;
		
		float angle = currentLayout.startPositionMarker.eulerAngles.y * Mathf.Deg2Rad;

		return angle;
	}

}
