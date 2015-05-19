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

	public enum LayoutErrorType {
		NONE,
		TOO_CLOSE,
		TOO_FAR,
		TOO_HIGH,
		TOO_LOW,
		WRONG_BLOCK,
		WRONG_STACK,
		WRONG_COLOR
	}

	[SerializeField] bool debug = false;
	[SerializeField] List<GameLayout> layouts;

	[SerializeField] GameObject layoutPreviewPanel = null;
	[SerializeField] GameObject hideDuringPreview = null;
	[SerializeField] Text previewTitle;
	[SerializeField] Button letsBuildButton;
	[SerializeField] GameObject hideWhenInventoryComplete;

	[SerializeField] GameObject layoutInstructionsPanel = null;
	[SerializeField] Camera layoutInstructionsCamera = null;
	[SerializeField] GameObject ghostPrefab = null;

	[SerializeField] GameObject block2dPrefab = null;
	[SerializeField] RectTransform block2dAnchor = null;

	[SerializeField] Text instructionsTitle;
	[SerializeField] Text textProgress;
	[SerializeField] Button buttonStartPlaying;
	[SerializeField] ScreenMessage screenMessage = null;
	[SerializeField] float coplanarFudge = 0.5f;
	[SerializeField] float distanceFudge = 0.5f;

	[SerializeField] AudioClip inventoryCompleteSound = null;

	[SerializeField] AudioClip blockValidatedSound = null;
	[SerializeField] AudioClip layoutValidatedSound = null;
	[SerializeField] AudioClip validPredictedDropSound = null;
	[SerializeField] AudioClip invalidPredictedDropSound = null;

	[SerializeField] Image image_cozmoTD;
	[SerializeField] LayoutBlock2d carriedBlock2d;
	[SerializeField] ChangeCubeModeButton button_change;
	//dmd todo add vector lines from carried button to layout cubes it could satisfy?


	List<BuildInstructionsCube> validated = new List<BuildInstructionsCube>();

	Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

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

	bool ignoreActiveColor = false;

	List<LayoutBlock2d> blocks2d = new List<LayoutBlock2d>();

	bool lastValidPredictedDrop = false;

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
			if(hideDuringPreview!= null) hideDuringPreview.SetActive(true);
			Phase = LayoutTrackerPhase.DISABLED;
			return;
		}

		SetUpInventory();

		string fullName = currentGameName + " #" + currentLevelNumber;
		previewTitle.text = fullName;
		instructionsTitle.text = fullName;

		if(letsBuildButton != null) letsBuildButton.gameObject.SetActive(false);
		if(hideWhenInventoryComplete != null) hideWhenInventoryComplete.SetActive(true);

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
		lastValidPredictedDrop = false;
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
				if(prior.Block.objectFamily == 3 && prior.Block.activeBlockMode != block.activeBlockMode && !ignoreActiveColor) continue;
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

		if(Phase == LayoutTrackerPhase.DISABLED) return;

		bool validPredictedDrop = false;

		if(validCount < currentLayout.blocks.Count && robot != null && robot.carryingObject != null) {
			string error;
			GameLayoutTracker.LayoutErrorType errorType;
			validPredictedDrop = PredictDropValidation(robot.carryingObject, out error, out errorType, false);

			if(lastValidPredictedDrop != validPredictedDrop) {
				audio.PlayOneShot(validPredictedDrop ? validPredictedDropSound : invalidPredictedDropSound);
			}
		}

		if(lastValidPredictedDrop != validPredictedDrop) {
			robot.SetBackpackLEDs(validPredictedDrop ? Color.green : Color.clear, Color.clear);
		}

		lastValidPredictedDrop = validPredictedDrop;

		if(robot != null && robot.carryingObject != null && robot.carryingObject.isActive) {



		}
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

		switch(Phase) {
			case LayoutTrackerPhase.DISABLED:
				layoutInstructionsPanel.SetActive(false);
				layoutPreviewPanel.SetActive(false);
				if(hideDuringPreview!= null) hideDuringPreview.SetActive(true);
				layoutInstructionsCamera.gameObject.SetActive(false);

				image_cozmoTD.gameObject.SetActive(false);
				carriedBlock2d.gameObject.SetActive(false);
				button_change.gameObject.SetActive(false);
				break;

			case LayoutTrackerPhase.INVENTORY:
				ShowAllBlocks();
				layoutInstructionsPanel.SetActive(false);
				layoutPreviewPanel.SetActive(!hidden);
				if(hideDuringPreview!= null) hideDuringPreview.SetActive(false);
				layoutInstructionsCamera.gameObject.SetActive(false);
				image_cozmoTD.gameObject.SetActive(false);
				carriedBlock2d.gameObject.SetActive(false);
				button_change.gameObject.SetActive(false);

				if(robot != null) robot.SetHeadAngle();

				bool inventoryComplete = true;

				for(int i=0;i<blocks2d.Count;i++) {
					int known = GetKnownObjectCountForBlock(blocks2d[i].Block);
					bool validated = known >= blocks2d[i].Dupe;
					blocks2d[i].Validate(validated);
					inventoryComplete &= validated;
				}

				if(letsBuildButton != null) {
					if(!letsBuildButton.gameObject.activeSelf && inventoryComplete) {
						audio.PlayOneShot(inventoryCompleteSound);
					}
					letsBuildButton.gameObject.SetActive(inventoryComplete);
				}

				if(hideWhenInventoryComplete != null) hideWhenInventoryComplete.SetActive(!inventoryComplete);

				break;

			case LayoutTrackerPhase.BUILDING:

				textProgress.text = validCount + " / " + currentLayout.blocks.Count;

				bool hideNow = hidden;

//				if(!hideNow && robot != null && robot.carryingObject != null && robot.carryingObject.isActive) {
//					hideNow = !IsActiveBlockCorrectColor(robot.carryingObject as ActiveBlock);
//				}

				layoutInstructionsPanel.SetActive(!hideNow);
				layoutPreviewPanel.SetActive(false);
				if(hideDuringPreview!= null) hideDuringPreview.SetActive(true);
				layoutInstructionsCamera.gameObject.SetActive(!hideNow);

//				if(robot != null) {
//					if(robot.carryingObject == null) {
						image_cozmoTD.gameObject.SetActive(false);
						carriedBlock2d.gameObject.SetActive(false);
						button_change.gameObject.SetActive(false);
//					}
//					else {
//						image_cozmoTD.gameObject.SetActive(robot.carryingObject.isActive);
//						carriedBlock2d.Initialize(robot.carryingObject);
//						carriedBlock2d.gameObject.SetActive(robot.carryingObject.isActive);
//						button_change.gameObject.SetActive(robot.carryingObject.isActive);
//					}
//				}

				break;

			case LayoutTrackerPhase.COMPLETE:
				textProgress.text = currentLayout.blocks.Count + " / " + currentLayout.blocks.Count;
				layoutInstructionsPanel.SetActive(!hidden);
				layoutPreviewPanel.SetActive(false);
				if(hideDuringPreview!= null) hideDuringPreview.SetActive(true);
				layoutInstructionsCamera.gameObject.SetActive(!hidden);
				carriedBlock2d.gameObject.SetActive(false);
				button_change.gameObject.SetActive(false);

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
						if(robot.carryingObject.isActive && currentLayout.blocks[i].objectFamily != 3) continue;
						if(!robot.carryingObject.isActive && currentLayout.blocks[i].objectType != (int)robot.carryingObject.ObjectType) continue;
						layoutCubeMatchingCarried = currentLayout.blocks[i];
						break;
					}
					if(layoutCubeMatchingCarried.objectFamily == 3 && robot.activeBlocks[robot.carryingObject].mode != layoutCubeMatchingCarried.activeBlockMode) {
						screenMessage.ShowMessage("Cozmo can now CHANGE this block's color to "+layoutCubeMatchingCarried.activeBlockMode.ToString()+".", Color.white);
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
				validate = true;
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
			layout.blocks[i].AssignedObject = null;
		}
	}

	string ValidateBlocks() {
		string error = "";
		validCount = 0;

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
			block.AssignedObject = null;
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

				//cannot validate block in hand
				if(newObject == robot.carryingObject) {
					continue;
				}

				if(block.objectFamily != (int)newObject.Family) {
					continue;
				}

				//skip objects of the wrong type
				if(block.objectFamily != 3 && block.objectType != (int)newObject.ObjectType) {
					if(debug) Debug.Log("skip objects of the wrong type");
					continue;
				}

				if(!ignoreActiveColor && block.objectFamily == 3 && block.activeBlockMode != robot.activeBlocks[newObject].mode) { //active block
					if(debug) Debug.Log("skip active block of the wrong color. goalColor("+block.activeBlockMode+") newObject("+newObject+"):color("+robot.activeBlocks[newObject].mode+")");
					continue;
				}

				//skip objects already assigned to a layout block
				if(layout.blocks.Find( x => x.AssignedObject == newObject) != null) continue;

				if(validated.Count == 0) {
					ValidateBlock(layoutBlockIndex, newObject);
					if(debug) Debug.Log("validated block("+block.gameObject.name+") of type("+block.objectType+") family("+block.objectFamily+") because first block placed on ground.");
				}
				//if this ideal block needs to get stacked on one we already know about...
				else if(block.cubeBelow != null) {

					ObservedObject objectToStackUpon = block.cubeBelow.AssignedObject;

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

						Vector3 idealOffset = CozmoUtil.Vector3UnityToCozmoSpace((block.transform.position - validated[validatedIndex].transform.position) / block.Size);

						ObservedObject priorObject = validated[validatedIndex].AssignedObject;
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

	void ValidateBlock(int layoutBlockIndex, ObservedObject newObject) {
		BuildInstructionsCube block = currentLayout.blocks[layoutBlockIndex];
		validated.Add(block);
		block.AssignedObject = newObject;
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
		ghostBlock.activeBlockMode = failingLayoutBlock.activeBlockMode;
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
			if(!obj.isActive && obj.ObjectType != block.objectType) continue;
			//if(obj.Family == 3 && obj.activeBlockMode != block.activeBlockMode) continue;

			count++;
		}

		return count;
	}

	public void DebugQuickValidate() {
		ignoreActiveColor = true;
		ValidateBlocks();
		ValidateBuild();

		Debug.Log("DebugQuickValidate validated("+validated.Count+")");
	}

	public void ValidateBuild() {
		Validated = true;
		Phase = LayoutTrackerPhase.DISABLED;
	}

	public void EndPreview() {
		Phase = LayoutTrackerPhase.BUILDING;
		ValidateBlocks();
	}

	public Vector3 GetStartingPositionFromLayout(out float facingAngle, out Vector3 facingVector) {
		facingAngle = 0f;
		facingVector = Vector3.zero;

		if(currentLayout == null) return Vector3.zero;
		if(currentLayout.startPositionMarker == null) return Vector3.zero;

		return GetPoseFromLayoutForTransform(currentLayout.startPositionMarker, out facingAngle, out facingVector, Vector3.zero);
	}

	public Vector3 GetPoseFromLayoutForTransform(Transform t, out float facingAngle, out Vector3 facingVector, Vector3 directionOverride) {
		facingAngle = 0f;
		facingVector = Vector3.zero;
		
		if(currentLayout == null) return Vector3.zero;
		if(t == null) return Vector3.zero;
		
		List<BuildInstructionsCube> layoutBlocksOnGround = currentLayout.blocks.FindAll(x => x.cubeBelow == null && x.Validated);
		
		BuildInstructionsCube layoutBlock1 = layoutBlocksOnGround[0];
		
		float scaleToCozmo = CozmoUtil.BLOCK_LENGTH_MM / layoutBlock1.Size;
		
		Vector3 offsetFromFirstBlock = CozmoUtil.Vector3UnityToCozmoSpace(t.position - layoutBlock1.transform.position) * scaleToCozmo;
		offsetFromFirstBlock.z = 0f;
		facingVector = CozmoUtil.Vector3UnityToCozmoSpace(t.forward).normalized;
		facingAngle = Vector3.Angle(Vector3.right, facingVector) * (Vector3.Dot(facingVector, Vector3.up) >= 0f ? 1f : -1f) * Mathf.Deg2Rad;
		
		//if layout has only one block for some reason, just use default rotation
		if(layoutBlocksOnGround.Count == 1) {
			//Debug.Log("GetStartingPositionFromLayout layoutBlocksOnGround.Count == 1 use default rotation.");
			if(directionOverride.sqrMagnitude > 0f) {
				offsetFromFirstBlock = offsetFromFirstBlock.magnitude * directionOverride.normalized;
			}
			return offsetFromFirstBlock + layoutBlock1.AssignedObject.WorldPosition;
		}
		
		BuildInstructionsCube layoutBlock2 = layoutBlocksOnGround[1];
		
		Vector3 layoutFirstToSecond = CozmoUtil.Vector3UnityToCozmoSpace(layoutBlock2.transform.position - layoutBlock1.transform.position) * scaleToCozmo;
		layoutFirstToSecond.z = 0f;
		
		if(layoutFirstToSecond.magnitude > CozmoUtil.BLOCK_LENGTH_MM * 0.25f) {
			
			Vector3 observedFirstToSecond = layoutBlock2.AssignedObject.WorldPosition - layoutBlock1.AssignedObject.WorldPosition;
			observedFirstToSecond.z = 0f;
			
			float offsetAngle = Vector3.Angle(layoutFirstToSecond, observedFirstToSecond);
			Vector3 axis = Vector3.Cross(layoutFirstToSecond.normalized, observedFirstToSecond.normalized);
			
			//float sign = Mathf.Sign(Vector3.Dot(Vector3.forward,axis));
			//offsetAngle *= sign;
			
			Quaternion observedRotation = Quaternion.AngleAxis(offsetAngle, axis);
			
			offsetFromFirstBlock = observedRotation * offsetFromFirstBlock;
			facingVector = observedRotation * facingVector;
			
			float signedAngleOffsetRad = Vector3.Angle(Vector3.right, facingVector) * (Vector3.Dot(facingVector, Vector3.up) >= 0f ? 1f : -1f) * Mathf.Deg2Rad;
			
			Debug.Log("GetStartingPositionFromLayout facingAngle("+facingAngle+") signedAngleRad("+signedAngleOffsetRad+") newFacing("+(facingAngle + signedAngleOffsetRad)+") axis.z("+axis.z+")");
			facingAngle = signedAngleOffsetRad;
			
			//Debug.Log("GetStartingPositionFromLayout rotating offset by offsetAngle("+offsetAngle+") axis("+axis+")");
			
		}
		
		Vector3 pose = offsetFromFirstBlock + layoutBlock1.AssignedObject.WorldPosition;
		
		return pose;
	}


	//we are dropping an object and want to know which blocks are already on the ground and valid
	//	so we can then do distance checks if necessaril to validate the placement of the drop
	bool IsValidGroundBlock(BuildInstructionsCube block) {
		if(!block.Validated) return false;
		if(block.cubeBelow != null) return false;
		
		return true;
	}

	//we are dropping an object and want to know which layout block it might be an apt match for
	bool IsUnvalidatedMatchingGroundBlock(BuildInstructionsCube block, ObservedObject objectToMatch) {
		if(block.Validated) return false;
		if(block.cubeBelow != null) return false;
		if(block.objectFamily != objectToMatch.Family) return false;
		if(!ignoreActiveColor && block.objectFamily == 3 && robot.activeBlocks[objectToMatch].mode != block.activeBlockMode) return false;
		if(block.objectFamily != 3 && objectToMatch.ObjectType != block.objectType) return false;
		return true;
	}

	bool IsUnvalidatedMatchingStackedBlock(BuildInstructionsCube block, ObservedObject objectToMatch) {
		if(block.Validated) return false;
		if(block.cubeBelow == null) return false;
		if(block.objectFamily != objectToMatch.Family) return false;
		if(!ignoreActiveColor && block.objectFamily == 3 && robot.activeBlocks[objectToMatch].mode != block.activeBlockMode) return false;
		if(block.objectFamily != 3 && objectToMatch.ObjectType != block.objectType) return false;
		return true;
	}

	bool IsActiveBlockCorrectColor(ActiveBlock activeBlockToMatch) {

		for(int i=0; i<currentLayout.blocks.Count; i++) {
			BuildInstructionsCube block = currentLayout.blocks[i];
			if(block.Validated) continue;
			if(block.objectFamily != 3) continue;
			if(activeBlockToMatch.mode == block.activeBlockMode) return true;
		}

		return false;
	}

	public bool AttemptAssistedPlacement( ObservedObject objectToPlace, out Vector3 pos, out float facing_rad) {
		pos = Vector3.zero;
		facing_rad = 0f;

		ValidateBlocks ();

		if(robot == null) return false;
		if(objectToPlace == null) return false;
		
		Vector3 posToDrop = robot.WorldPosition + robot.Forward * CozmoUtil.BLOCK_LENGTH_MM + Vector3.forward * CozmoUtil.BLOCK_LENGTH_MM * 0.5f;

		ignoreActiveColor = true;

		List<BuildInstructionsCube> newBlocks = currentLayout.blocks.FindAll(x => IsUnvalidatedMatchingGroundBlock(x, objectToPlace));
		
		if(newBlocks == null || newBlocks.Count == 0) {
			//this is probably ok?  may need to do more processing to see if its ok

			newBlocks = currentLayout.blocks.FindAll(x => IsUnvalidatedMatchingStackedBlock(x, objectToPlace));
			if(newBlocks == null || newBlocks.Count == 0) {
				Debug.Log ("This block is not required in this layout.");
				return false;
			}
			else {
				return AttemptAssistedStack(objectToPlace, newBlocks);
			}
		}

		
		List<BuildInstructionsCube> priorBlocks = currentLayout.blocks.FindAll(x => IsValidGroundBlock(x));
		
		if(priorBlocks == null || priorBlocks.Count == 0) {
			//if this will be our first ground block to validate, automatically valid location
			Debug.Log ("No prior Blocks.");
			return false;
		}

		BuildInstructionsCube bestBlock = newBlocks[0];
		float leastError = float.MaxValue;

		//go through each not yet validated layout block this object could work for and see if it a legal position
		// where legal positon is determined by a fudgey distance check to all prior validated ground blocks
		for(int newIndex=0; newIndex < newBlocks.Count; newIndex++) {
			
			BuildInstructionsCube block = newBlocks[newIndex];
			float totalDistanceError = 0;
			for(int priorIndex=0; priorIndex < priorBlocks.Count; priorIndex++) {
				
				BuildInstructionsCube priorBlock = priorBlocks[priorIndex];
				
				Vector3 idealOffset = CozmoUtil.Vector3UnityToCozmoSpace((block.transform.position - priorBlock.transform.position) / block.Size);
				
				ObservedObject priorObject = priorBlock.AssignedObject;
				Vector3 realOffset = (posToDrop - priorObject.WorldPosition) / CozmoUtil.BLOCK_LENGTH_MM;

				float idealDistance = ((Vector2)idealOffset).magnitude;
				float realDistance = ((Vector2)realOffset).magnitude;

				totalDistanceError += Mathf.Abs(realDistance - idealDistance);
			}

			if(totalDistanceError < leastError) {
				bestBlock = newBlocks[newIndex];
				leastError = totalDistanceError;
			}
		}

		Vector3 closestFace = priorBlocks [0].AssignedObject.GetBestFaceVector ((Vector3)(posToDrop - priorBlocks [0].AssignedObject.WorldPosition));

		Vector3 facingVector;
		pos = GetPoseFromLayoutForTransform(bestBlock.transform, out facing_rad, out facingVector, closestFace);

		if (bestBlock.objectFamily == 3) {
			ActiveBlock activeBlock = objectToPlace as ActiveBlock;
			if(activeBlock.mode != bestBlock.activeBlockMode) {
				activeBlock.mode = bestBlock.activeBlockMode;
				activeBlock.SetLEDs( CozmoPalette.instance.GetUIntColorForActiveBlockType( activeBlock.mode ) );
			}
		}

		robot.DropObjectAtPose(pos, facing_rad);

		return true;
	}

	public bool AttemptAssistedStack( ObservedObject objectToStack, List<BuildInstructionsCube> potentiallyStackable)  {

		List<BuildInstructionsCube> potentiallyStackedUpon = currentLayout.blocks.FindAll (x => x.cubeBelow == null && x.cubeAbove != null && !x.cubeAbove.Validated); //newBlocks.Find ( y => y.cubeBelow == x ) != null );
		
		if(potentiallyStackedUpon == null || potentiallyStackedUpon.Count == 0) {
			//if this will be our first ground block to validate, automatically valid location
			Debug.Log ("No prior Blocks.");
			return false;
		}

		float closestRange = float.MaxValue;

		BuildInstructionsCube layoutBlockToStack = potentiallyStackable [0];
		ObservedObject objectToStackUpon = potentiallyStackedUpon [0].AssignedObject;
		if(objectToStackUpon == null) {
			objectToStackUpon = robot.knownObjects.Find ( x => IsUnvalidatedMatchingGroundBlock(potentiallyStackedUpon [0], x) );
		}

		for(int i=0;i<potentiallyStackedUpon.Count;i++) {
			ObservedObject obj = potentiallyStackedUpon[i].AssignedObject;
			if(obj == null) {
				obj = robot.knownObjects.Find ( x => IsUnvalidatedMatchingGroundBlock(potentiallyStackedUpon[i], x) );
			}
			if(obj == null) continue;

			float range = (obj.WorldPosition - robot.WorldPosition).magnitude;
			if(range < closestRange) {
				BuildInstructionsCube layoutBlock = potentiallyStackedUpon[i].cubeAbove;
				if(layoutBlock != null) {
					closestRange = range;
					layoutBlockToStack = layoutBlock;
				}
			}
		}

		if(objectToStack.isActive) {
			ActiveBlock activeBlock = objectToStack as ActiveBlock;
			if(activeBlock.mode != layoutBlockToStack.activeBlockMode) {
				activeBlock.mode = layoutBlockToStack.activeBlockMode;
				activeBlock.SetLEDs( CozmoPalette.instance.GetUIntColorForActiveBlockType( activeBlock.mode ) );
			}
		}

		robot.PickAndPlaceObject( objectToStackUpon );
		if(CozmoBusyPanel.instance != null)	{
			
			if(objectToStack != null) {
				string desc = "Cozmo is attempting to stack\n";
				
				if(objectToStack.isActive) {
					desc += "an Active Block";
				}
				else {
					desc += "a ";
					
					if(CozmoPalette.instance != null) {
						desc += CozmoPalette.instance.GetNameForObjectType((int)objectToStack.ObjectType) + " ";
					}
					
					desc += "Block";
				}
				
				if(objectToStackUpon != null) {
					
					desc += "\n on top of ";
					
					if(objectToStackUpon.isActive) {
						desc += "an Active Block";
					}
					else {
						desc += "a ";
						
						if(CozmoPalette.instance != null) {
							desc += CozmoPalette.instance.GetNameForObjectType((int)objectToStackUpon.ObjectType) + " ";
						}
						
						desc += "Block";
					}
				}
				
				desc += ".";
				
				CozmoBusyPanel.instance.SetDescription(desc);
			}
		}

		return true;
	}


	public bool PredictDropValidation( ObservedObject objectToDrop, out string errorText, out LayoutErrorType errorType, bool approveUnecessaryDropping=true) {
		errorText = "";
		errorType = LayoutErrorType.NONE;

		if(robot == null) return false;
		if(objectToDrop == null) return false;

		Vector3 posToDrop = robot.WorldPosition + robot.Forward * CozmoUtil.BLOCK_LENGTH_MM + Vector3.forward * CozmoUtil.BLOCK_LENGTH_MM * 0.5f;

		List<BuildInstructionsCube> newBlocks = currentLayout.blocks.FindAll(x => IsUnvalidatedMatchingGroundBlock(x, objectToDrop));

		if(newBlocks == null || newBlocks.Count == 0) {
			//this is probably ok?  may need to do more processing to see if its ok
			//error = "No block of this type should be placed here.";
			return approveUnecessaryDropping;
		}

		List<BuildInstructionsCube> priorBlocks = currentLayout.blocks.FindAll(x => IsValidGroundBlock(x));

		if(priorBlocks == null || priorBlocks.Count == 0) {
			//if this will be our first ground block to validate, automatically valid location
			return true;
		}

		bool failDistanceCheck = false;

		//go through each not yet validated layout block this object could work for and see if it a legal position
			// where legal positon is determined by a fudgey distance check to all prior validated ground blocks
		for(int newIndex=0; newIndex < newBlocks.Count; newIndex++) {

			BuildInstructionsCube block = newBlocks[newIndex];

			for(int priorIndex=0; priorIndex < priorBlocks.Count; priorIndex++) {

				BuildInstructionsCube priorBlock = priorBlocks[priorIndex];

				Vector3 idealOffset = CozmoUtil.Vector3UnityToCozmoSpace((block.transform.position - priorBlock.transform.position) / block.Size);

				ObservedObject priorObject = priorBlock.AssignedObject;
				Vector3 realOffset = (posToDrop - priorObject.WorldPosition) / CozmoUtil.BLOCK_LENGTH_MM;
				
				//are we basically on the same plane and roughly the correct distance away?
				if(Mathf.Abs(realOffset.z) > ( coplanarFudge * 0.9f )) {
					errorText = "Drop position is too " + ( realOffset.z > 0f ? "high" : "low" ) + ".";
					errorType = realOffset.z > 0f ? LayoutErrorType.TOO_HIGH : LayoutErrorType.TOO_LOW;
					failDistanceCheck = true;
					break;
				}
				
				float idealDistance = ((Vector2)idealOffset).magnitude;
				float realDistance = ((Vector2)realOffset).magnitude;
				
				float distanceError = realDistance - idealDistance;
				if(Mathf.Abs(distanceError) > ( distanceFudge * 0.9f )) {
					errorText = "Drop position is too " + ( distanceError > 0f ? "far" : "close" ) + ".";
					errorType = distanceError > 0f? LayoutErrorType.TOO_FAR : LayoutErrorType.TOO_CLOSE;
					failDistanceCheck = true;
					break;
				}
			}

			//we only need to satisfy one potential new block's placement for this to be a valid drop
			if(!failDistanceCheck) break;
		}

		return !failDistanceCheck;
	}

	public bool PredictStackValidation( ObservedObject objectToStack, ObservedObject objectToStackUpon, out string errorText, out LayoutErrorType errorType) {

		errorText = "";
		errorType = LayoutErrorType.NONE;

		BuildInstructionsCube layoutBlockToStackUpon = currentLayout.blocks.Find(x => x.AssignedObject == objectToStackUpon);
		if(layoutBlockToStackUpon == null) {
			layoutBlockToStackUpon = currentLayout.blocks.Find(x => IsUnvalidatedMatchingGroundBlock(x, objectToStackUpon) );
		
			if(layoutBlockToStackUpon == null) {
				//this is probably ok?  may need to do more processing to see if its ok
				errorText = "You are attempting to stack upon the wrong block.";
				errorType = LayoutErrorType.WRONG_STACK;
				return false;
			}
		}

		BuildInstructionsCube layoutBlockToStack = currentLayout.blocks.Find(x => x.cubeBelow == layoutBlockToStackUpon);
		if(layoutBlockToStack == null) {
			errorText = "You are attempting to stack upon the wrong block.";
			errorType = LayoutErrorType.WRONG_STACK;
			return false;
		}

		if(layoutBlockToStack.Validated) {
			errorText = "A valid block is already stacked there.";
			errorType = LayoutErrorType.WRONG_STACK;
			return false;
		}

		if(layoutBlockToStack.objectFamily == 3) {

			if(layoutBlockToStack.objectFamily != objectToStack.Family) {
				errorText = "You are attempting to stack a non active block where an active block belongs.";
				errorType = LayoutErrorType.WRONG_BLOCK;
				return false;
			}

			if(robot.activeBlocks[objectToStack].mode != layoutBlockToStack.activeBlockMode && !ignoreActiveColor) {
				errorText = "This active block needs to be "+layoutBlockToStack.activeBlockMode+" before it is stacked.";
				errorType = LayoutErrorType.WRONG_COLOR;
				return false;
			}
		}

		if(layoutBlockToStack.objectFamily != 3) {

			if(layoutBlockToStack.objectFamily != objectToStack.Family) {
				errorText = "You are attempting to stack an active block where an standard block belongs.";
				errorType = LayoutErrorType.WRONG_BLOCK;
				return false;
			}

			if(objectToStack.ObjectType != layoutBlockToStack.objectType) {
				errorText = "You are attempting to stack a " +CozmoPalette.instance.GetNameForObjectType((int)objectToStack.ObjectType)+ " block where a "+CozmoPalette.instance.GetNameForObjectType(layoutBlockToStack.objectType)+" block belongs.";
				errorType = LayoutErrorType.WRONG_BLOCK;
				return false;
			}
		}

		return true;
	}

	public void SetMessage(string message, Color color) {

		screenMessage.ShowMessage(message, color);

	}

	public List<ObservedObject> GetTrackedObjectsInOrder() {
		List<ObservedObject> objects = new List<ObservedObject>();

		if(currentLayout != null) {
			for(int i=0;i<currentLayout.blocks.Count;i++) {
				if(!currentLayout.blocks[i].Validated) continue;
				if(currentLayout.blocks[i].AssignedObject == null) continue;
				objects.Add(currentLayout.blocks[i].AssignedObject);
			}
		}

		Debug.Log("GetTrackedObjectsInOrder returning " + objects.Count + " objects.");
		return objects;
	}

}
