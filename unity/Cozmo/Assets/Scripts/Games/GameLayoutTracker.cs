using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using Anki.Cozmo;
using G2U = Anki.Cozmo.G2U;
using U2G = Anki.Cozmo.U2G;

public enum LayoutTrackerPhase {
	DISABLED,
	INVENTORY,
	AUTO_BUILDING,
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

public class GameLayoutTracker : MonoBehaviour {

	//dmd todo add vector lines from carried button to layout cubes it could satisfy?

	#region INSPECTOR FIELDS

	[SerializeField] bool debug = false;
	[SerializeField] List<GameLayout> layouts;

	[SerializeField] GameObject inventoryPanel = null;
	[SerializeField] Text inventoryTitle;
	[SerializeField] Button button_manualBuild;
	[SerializeField] Button button_autoBuild;
	[SerializeField] Image image_localizedCheck;
	[SerializeField] GameObject inventoryHints;  //hide when inventory completed
	[SerializeField] GameObject block2dPrefab = null;
	[SerializeField] RectTransform block2dAnchor = null;
	[SerializeField] AudioClip inventoryCompleteSound = null;

	[SerializeField] GameObject layoutInstructionsPanel = null;
	[SerializeField] Camera layoutInstructionsCamera = null;
	[SerializeField] GameObject ghostPrefab = null;
	[SerializeField] Text instructionsTitle;
	[SerializeField] Text textProgress;
	[SerializeField] Button buttonStartPlaying;
	[SerializeField] ScreenMessage screenMessage = null;

	[SerializeField] float coplanarFudge = 0.5f;
	[SerializeField] float distanceFudge = 0.5f;
	[SerializeField] float angleFudge = 10f;

	[SerializeField] AudioClip blockValidatedSound = null;
	[SerializeField] AudioClip layoutValidatedSound = null;
	[SerializeField] AudioClip validPredictedDropSound = null;
	[SerializeField] AudioClip invalidPredictedDropSound = null;
	[SerializeField] AudioClip cubePlaced = null;
	[SerializeField] AudioClip buildComplete = null;

	[SerializeField] int cycleCount = 5;
	[SerializeField] float cycleTime = 0.2f;
	[SerializeField] float cycleDelayTime = 0.4f;
	[SerializeField] AudioClip cycleSound;
	[SerializeField] AudioClip cycleSuccessSound;

	#endregion

	#region MISC MEMBERS
	
	public static GameLayoutTracker instance = null;
	
	public bool Validated { get; private set; }

	public LayoutTrackerPhase Phase { get; private set; }

	Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	bool blocksInitialized = false;
	BuildInstructionsCube ghostBlock;

	GameLayout currentLayout = null;
	string currentGameName = "Unknown";
	int currentLevelNumber = 1;
	int validCount = 0;
	int lastValidCount = 0;
	bool hidden = false;
	bool ignoreActiveColor = false;
	List<LayoutBlock2d> blocks2d = new List<LayoutBlock2d>();
	bool lastValidPredictedPlacement = false;
	int autoBuildFails = 0;
	
	List<BuildInstructionsCube> validated = new List<BuildInstructionsCube>();
	List<ObservedObject> potentialObservedObjects = new List<ObservedObject>();
	List<ObservedObject> inventory = new List<ObservedObject>();
	List<ObservedObject> unplacedObjects = new List<ObservedObject>();
	bool ignoreCoplanerConstraintsOnDrop = true;

	bool iStartGame = false;
	bool iStartBuild = false;
	bool iStartAutoBuild = false;

	bool skipBuildForThisLayout = false;

	#endregion

	#region COMPONENT CALLBACKS

	void OnEnable () {

		instance = this;

		if(!blocksInitialized) {
			for(int i=0; i<layouts.Count; i++) {
				layouts[i].Initialize();
			}
			blocksInitialized = true;
		}

		currentGameName = PlayerPrefs.GetString("CurrentGame", "Unknown");
		currentLevelNumber = PlayerPrefs.GetInt(currentGameName + "_CurrentLevel", 1);

		currentLayout = null;
		for(int i=0; i<layouts.Count; i++) {
			layouts[i].gameObject.SetActive(layouts[i].gameName == currentGameName && layouts[i].levelNumber == currentLevelNumber);

			if(layouts[i].gameObject.activeSelf) {
				currentLayout = layouts[i];
			}
		}

		//no apt layout found?  then just disable
		if(currentLayout == null) {
			gameObject.SetActive(false);
			return;
		}

		skipBuildForThisLayout = currentLayout.blocks.Find( x => x.isHeld ) != null;

		Phase = LayoutTrackerPhase.INVENTORY;

		ClearFrameWiseInputs();

		validCount = 0;
		lastValidCount = 0;
		lastValidPredictedPlacement = false;
		hidden = false;

		string fullName = currentGameName + " #" + currentLevelNumber;
		inventoryTitle.text = fullName;
		instructionsTitle.text = fullName;

		Enter_INVENTORY();

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
		
		if(revalidateThisFrame || Input.GetKeyDown(KeyCode.V)) {
			AnalyzeLayoutForValidation();
		}

		LayoutTrackerPhase nextPhase = GetNextPhase();

		if(nextPhase != Phase) {
			ExitPhase();
			Phase = nextPhase;
			EnterPhase();	
		}
		else {
			UpdatePhase();
		}

		ClearFrameWiseInputs();
	}
	
	void OnDisable() {

		ExitPhase();

		if(instance == this) instance = null;
		
		if(ghostBlock != null) ghostBlock.gameObject.SetActive(false);
	
		inventoryPanel.SetActive(false);
		layoutInstructionsPanel.SetActive(false);
		layoutInstructionsCamera.gameObject.SetActive(false);
		
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure -= SuccessOrFailure;
	}
	
	#endregion

	#region PRIVATE METHODS

	void ClearFrameWiseInputs() {
		iStartGame = false;
		iStartBuild = false;
		iStartAutoBuild = false;
		revalidateThisFrame = false;
	}

	LayoutTrackerPhase GetNextPhase() {
		if(currentLayout == null) return LayoutTrackerPhase.DISABLED;
		if(iStartGame) {
			Debug.Log("GetNextPhase if(iStartGame) return LayoutTrackerPhase.DISABLED;");
			return LayoutTrackerPhase.DISABLED;
		}

		bool completed = validCount == currentLayout.blocks.Count;

		if(iStartBuild) return LayoutTrackerPhase.BUILDING;
		if(iStartAutoBuild) return LayoutTrackerPhase.AUTO_BUILDING;

		switch(Phase) {
			case LayoutTrackerPhase.INVENTORY:
				break;
			case LayoutTrackerPhase.AUTO_BUILDING:

				if(completed) return LayoutTrackerPhase.COMPLETE;
				break;
			case LayoutTrackerPhase.BUILDING:

				if(skipBuildForThisLayout) {
					BuildInstructionsCube unfinished = currentLayout.blocks.Find( x => !x.Validated && !x.isHeld );
					bool skipBuild = unfinished == null;
					if(skipBuild) {
						return LayoutTrackerPhase.DISABLED;
					}
					else {
						Debug.Log("unfinished("+unfinished.name+")");
					}
				}

				if(completed) return LayoutTrackerPhase.COMPLETE;
				break;
			case LayoutTrackerPhase.COMPLETE:

				break;
			case LayoutTrackerPhase.DISABLED:
				break;
		}

		return Phase;
	}

	void EnterPhase() {
		Debug.Log("EnterPhase("+Phase+")");

		switch(Phase) {
			case LayoutTrackerPhase.INVENTORY:
				Enter_INVENTORY();
				break;
			case LayoutTrackerPhase.AUTO_BUILDING:
				Enter_AUTO_BUILDING();
				break;
			case LayoutTrackerPhase.BUILDING:
				Enter_BUILDING();
				break;
			case LayoutTrackerPhase.COMPLETE:
				Enter_COMPLETE();
				break;
			case LayoutTrackerPhase.DISABLED:
				Enter_DISABLED();
				break;
		}
	}

	void UpdatePhase() {
		//Debug.Log("UpdatePhase("+Phase+")");

		switch(Phase) {
			case LayoutTrackerPhase.INVENTORY:
				Update_INVENTORY();
				break;
			case LayoutTrackerPhase.AUTO_BUILDING:
				Update_AUTO_BUILDING();
				break;
			case LayoutTrackerPhase.BUILDING:
				Update_BUILDING();
				break;
			case LayoutTrackerPhase.COMPLETE:
				Update_COMPLETE();
				break;
			case LayoutTrackerPhase.DISABLED:
				Update_DISABLED();
				break;
		}
	}

	void ExitPhase() {
		Debug.Log("ExitPhase("+Phase+")");

		switch(Phase) {
			case LayoutTrackerPhase.INVENTORY:
				Exit_INVENTORY();
				break;
			case LayoutTrackerPhase.AUTO_BUILDING:
				Exit_AUTO_BUILDING();
				break;
			case LayoutTrackerPhase.BUILDING:
				Exit_BUILDING();
				break;
			case LayoutTrackerPhase.COMPLETE:
				Exit_COMPLETE();
				break;
			case LayoutTrackerPhase.DISABLED:
				Exit_DISABLED();
				break;
		}
	}

	void Enter_INVENTORY() {

		if(robot != null) robot.ClearAllObjects();

		inventory.Clear ();
		blocks2d.Clear();
		
		int count = currentLayout.blocks.Count;
		for(int i=0; i<count; i++) {
			BuildInstructionsCube block = currentLayout.blocks[i];
			GameObject block2dObj = (GameObject)GameObject.Instantiate(block2dPrefab);
			RectTransform blockTrans = block2dObj.transform as RectTransform;
			blockTrans.SetParent(block2dAnchor, false);
			LayoutBlock2d block2d = block2dObj.GetComponent<LayoutBlock2d>();
			
			//count any duplicates so this one knows when to be validated
			int num = 1;
			foreach(LayoutBlock2d prior in blocks2d) {
				if(prior.Block.cubeType != block.cubeType) continue;
				if(!ignoreActiveColor && prior.Block.isActive && prior.Block.activeBlockMode != block.activeBlockMode) continue;
				num++;
			}
			
			block2d.Initialize(block, num);
			
			blocks2d.Add(block2d);
		}

		if(button_manualBuild != null) button_manualBuild.gameObject.SetActive(false);
		if(button_autoBuild != null) button_autoBuild.gameObject.SetActive(false);
		if(image_localizedCheck != null) image_localizedCheck.gameObject.SetActive(false);
		if(inventoryHints != null) inventoryHints.SetActive(true);
		inventoryPanel.SetActive(true);
		layoutInstructionsCamera.gameObject.SetActive(false);
		layoutInstructionsPanel.SetActive(false);

		ShowAllBlocks();

	}
	
	void Update_INVENTORY() {
		int lastInventoryCount = inventory.Count;
		inventoryPanel.SetActive(!hidden);

		bool inventoryComplete = true;
		
		for(int i=0;i<blocks2d.Count;i++) {
			ObservedObject inventoriedObject = GetKnownObjectForInventorySlot(blocks2d[i].Block, blocks2d[i].Dupe);
			bool validated = inventoriedObject != null;
			blocks2d[i].Validate(validated);
			inventoryComplete &= validated;
			
			if( inventory.Contains(inventoriedObject) != validated ) {
				
				if(validated) {
					if(inventoriedObject.isActive) {
						ActiveBlock activeBlock = inventoriedObject as ActiveBlock;
						activeBlock.SetLEDs( CozmoPalette.instance.GetUIntColorForActiveBlockType(ActiveBlock.Mode.White), 0, 255, 1000, ActiveBlock.Light.FOREVER);
					}
					
					inventory.Add(inventoriedObject);
				}  
				else {
					inventory.Remove(inventoriedObject);
				}
			}
		}
		
		//look down if not localized
		if(!inventoryComplete && robot != null) {
			if(/*!skipBuildForThisLayout && */!robot.IsLocalized()) {
				//Debug.Log( "look down because not localized" );
				
				robot.SetHeadAngle();
			}
			else {

				if(inventory.Count > lastInventoryCount) { //look at last seen object if any seen
					//Debug.Log( "look at last seen object if any seen: TrackHeadToObject " + inventory[inventory.Count-1] );
					float arc = 180f / currentLayout.blocks.Count;
					Vector3 latestPos = inventory[inventory.Count-1].WorldPosition;
					Vector2 toLatest = latestPos - robot.WorldPosition;
					float angle = MathUtil.SignedVectorAngle(robot.Forward, toLatest.normalized, Vector3.forward) + arc;
					Vector3 idealFacing = Quaternion.AngleAxis(angle, Vector3.forward) * robot.Forward;
					Vector3 facePosition = robot.WorldPosition + idealFacing * CozmoUtil.BLOCK_LENGTH_MM * 10f;

					angle *= Mathf.Deg2Rad;
	
					robot.TurnInPlace(angle);
			
					RobotEngineManager.instance.VisualizeQuad(33, CozmoPalette.ColorToUInt(Color.blue), robot.WorldPosition, robot.WorldPosition, latestPos, latestPos);
					RobotEngineManager.instance.VisualizeQuad(34, CozmoPalette.ColorToUInt(Color.magenta), robot.WorldPosition, robot.WorldPosition, facePosition, facePosition);
				}

				//Debug.Log( "look straight ahead to see objects" );
				
				robot.SetHeadAngle(0f);
			}
		}
		
		if(button_manualBuild != null) {
			if(!button_manualBuild.gameObject.activeSelf && inventoryComplete) {
				AudioManager.PlayOneShot(inventoryCompleteSound);
			}
			button_manualBuild.gameObject.SetActive(inventoryComplete);
		}
		
		
		if(image_localizedCheck != null) image_localizedCheck.gameObject.SetActive(robot.IsLocalized());
		if(button_autoBuild != null) button_autoBuild.gameObject.SetActive(!skipBuildForThisLayout && inventoryComplete && robot.IsLocalized());
		if(inventoryHints != null) inventoryHints.SetActive(!inventoryComplete);
	}
	void Exit_INVENTORY() {
		inventoryPanel.SetActive(false);
	}

	void Enter_AUTO_BUILDING() {
		ObservedObject.SignificantChangeDetected += SignificantChangeDetectedInObservedObject;
		autoBuildFails = 0;
		AnalyzeLayoutForValidation();
		buttonStartPlaying.gameObject.SetActive(false);
	}
	void Update_AUTO_BUILDING() {
		
		textProgress.text = validCount + " / " + currentLayout.blocks.Count;
		
		layoutInstructionsPanel.SetActive(!hidden);
		layoutInstructionsCamera.gameObject.SetActive(!hidden);
		
		if(validCount != currentLayout.blocks.Count && robot != null) {
			
			if(robot.isBusy) {
				//coz is doing shit
			}
			else if(robot.carryingObject != null) {
				Debug.Log("frame("+Time.frameCount+") AUTO_BUILDING AttemptAssistedPlacement");
				AttemptAssistedPlacement();
			}
			else {
				ObservedObject nextObject = GetObservedObjectForNextLayoutBlock();
				if(nextObject != null) {
					string description = null;
					CozmoBusyPanel.instance.SetDescription( "pick-up\n", nextObject, ref description );
					Debug.Log("frame("+Time.frameCount+") AUTO_BUILDING GetObservedObjectForNextLayoutBlock nextObject("+nextObject+")");
					robot.PickAndPlaceObject(nextObject);
				}
				else {
					Debug.Log("frame("+Time.frameCount+") GetObservedObjectForNextLayoutBlock could not find an apt nextObject to place.");
				}
			}
			RefreshValidationSounds();
			RefreshDropLocationHint();
		}

	}
	void Exit_AUTO_BUILDING() {
		layoutInstructionsPanel.SetActive(false);
		layoutInstructionsCamera.gameObject.SetActive(false);
		ObservedObject.SignificantChangeDetected -= SignificantChangeDetectedInObservedObject;
	}	

	void Enter_BUILDING() {
		ObservedObject.SignificantChangeDetected += SignificantChangeDetectedInObservedObject;
		AnalyzeLayoutForValidation();
		buttonStartPlaying.gameObject.SetActive(false);
	}
	void Update_BUILDING() {
		textProgress.text = validCount + " / " + currentLayout.blocks.Count;
		layoutInstructionsPanel.SetActive(!hidden);
		layoutInstructionsCamera.gameObject.SetActive(!hidden);

		RefreshValidationSounds();
		RefreshDropLocationHint();
	}
	void Exit_BUILDING() {
		layoutInstructionsPanel.SetActive(false);
		layoutInstructionsCamera.gameObject.SetActive(false);
		ObservedObject.SignificantChangeDetected -= SignificantChangeDetectedInObservedObject;
	}

	void Enter_COMPLETE() {
		int blockCount = currentLayout.blocks.Count;
		textProgress.text = blockCount.ToString() + " / " + blockCount.ToString();
		layoutInstructionsPanel.SetActive(!hidden);
		layoutInstructionsCamera.gameObject.SetActive(!hidden);

		buttonStartPlaying.gameObject.SetActive(true);
		AudioManager.PlayAudioClip(buildComplete, 0, AudioManager.Source.Notification);
	}
	void Update_COMPLETE() {
		layoutInstructionsPanel.SetActive(!hidden);
		layoutInstructionsCamera.gameObject.SetActive(!hidden);
	}
	void Exit_COMPLETE() {
		layoutInstructionsPanel.SetActive(false);
		layoutInstructionsCamera.gameObject.SetActive(false);
		buttonStartPlaying.gameObject.SetActive(false);
	}

	void Enter_DISABLED() {
		inventoryPanel.SetActive(false);
		layoutInstructionsPanel.SetActive(false);
		layoutInstructionsCamera.gameObject.SetActive(false);
		buttonStartPlaying.gameObject.SetActive(false);
	}
	void Update_DISABLED() {}
	void Exit_DISABLED() {}

	void RefreshDropLocationHint() {
		
		bool validPredictedPlacement = false;
		bool shouldBeStackedInstedOfDropped = false;
		uint colorCode = CozmoPalette.ColorToUInt(Color.clear);

		if(validCount < currentLayout.blocks.Count && robot != null && robot.carryingObject != null) {
			string error;
			LayoutErrorType errorType;
			
			validPredictedPlacement = PredictDropValidation(robot.carryingObject, out error, out errorType, out shouldBeStackedInstedOfDropped);
			
			if(shouldBeStackedInstedOfDropped && robot.selectedObjects.Count > 0) {
				validPredictedPlacement = PredictStackValidation(robot.carryingObject,robot.selectedObjects[0], out error, out errorType, true);
			}

			if(lastValidPredictedPlacement != validPredictedPlacement) {
				AudioManager.PlayOneShot(validPredictedPlacement ? validPredictedDropSound : invalidPredictedDropSound);
			}
		}

		if(validPredictedPlacement) {
			colorCode = CozmoPalette.ColorToUInt(Color.green);
		}
		else if(shouldBeStackedInstedOfDropped && robot.selectedObjects.Count > 0) {
			colorCode = CozmoPalette.ColorToUInt(Color.red);
		}

		robot.SetBackpackLEDs(colorCode);

		lastValidPredictedPlacement = validPredictedPlacement;

	}

	void RefreshValidationSounds() {
		
		if(validCount == currentLayout.blocks.Count) {
			if(lastValidCount != currentLayout.blocks.Count) {
				AudioManager.PlayOneShot(layoutValidatedSound);
			}
		}
		else if(validCount > lastValidCount) {
			AudioManager.PlayOneShot(blockValidatedSound);
		}
		
		lastValidCount = validCount;

	}

	bool revalidateThisFrame = false;
	void SignificantChangeDetectedInObservedObject() {
		revalidateThisFrame = true;
		//if(Time.frameCount == lastFrameAnalysis) return;
		//lastFrameAnalysis = Time.frameCount;
		Debug.Log("SignificantChangeDetectedInObservedObject, revalidating!");
		//AnalyzeLayoutForValidation();
	}

	void SuccessOrFailure(bool success, RobotActionType action_type) {
		if(Phase != LayoutTrackerPhase.BUILDING && Phase != LayoutTrackerPhase.AUTO_BUILDING) return;

		bool validate = false;

		switch(action_type) {
			case RobotActionType.UNKNOWN://	ACTION_UNKNOWN = -1,
				break;
			case RobotActionType.DRIVE_TO_POSE:
				break;
			case RobotActionType.DRIVE_TO_OBJECT:
				break;
			case RobotActionType.DRIVE_TO_PLACE_CARRIED_OBJECT:
				validate = true;
				break;
			case RobotActionType.TURN_IN_PLACE:
				break;
			case RobotActionType.MOVE_HEAD_TO_ANGLE:
				break;
			case RobotActionType.PICKUP_OBJECT_LOW:
			case RobotActionType.PICKUP_OBJECT_HIGH:
				if(robot.carryingObject != null && robot.carryingObject.isActive) {
					ActiveBlock activeBlock = robot.carryingObject as ActiveBlock;
					SetLightCubeToCorrectColor(activeBlock);
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
			if(Phase == LayoutTrackerPhase.AUTO_BUILDING) {
				autoBuildFails++;
				if(autoBuildFails >= 3) {
					if(!Application.isEditor) {
						StartBuild();
					}
					else {
						Debug.Log("frame("+Time.frameCount+") SuccessOrFailure autoBuildFails("+autoBuildFails+")");
					}
				}
			}
		}
		else {
			//if we succeed, reset our failure count?  this could be problematic if we are occilating between a succeed and fail
			autoBuildFails = 0;
		}

		if(validate) {
			AnalyzeLayoutForValidation();
			Debug.Log("frame("+Time.frameCount+") SuccessOrFailure success("+success+") lastValidCount("+lastValidCount+")->validCount("+validCount+")");
		}
	}

	void ShowAllBlocks() {
		GameLayout layout = currentLayout;
		if(layout == null) return;

		for(int i=0; i<layout.blocks.Count; i++) {
			layout.blocks[i].Hidden = false;
			layout.blocks[i].Highlighted = false;
			layout.blocks[i].Validated = false;
			layout.blocks[i].AssignedObject = null;
		}
	}

	void AnalyzeLayoutForValidation() {
		validCount = 0;
		validated.Clear();

		if(robot == null) return;

		GameLayout layout = currentLayout;
		if(layout == null) return;
		
		potentialObservedObjects.Clear();
		
		if(robot.knownObjects.Count == 0) return;
		
		potentialObservedObjects.AddRange(robot.knownObjects);

		//first loop through and clear our old assignments
		for(int layoutBlockIndex=0; layoutBlockIndex<layout.blocks.Count; layoutBlockIndex++) {
			BuildInstructionsCube block = layout.blocks[layoutBlockIndex];
//			if(block.Validated && potentialObservedObjects.Contains(block.AssignedObject)) {
//				potentialObservedObjects.Remove(block.AssignedObject);
//				validated.Add(block);
//			}
			block.Hidden = false;
			block.Highlighted = true;
			//block.Validated = false;
			//block.AssignedObject = null;
		}
		
		if(debug) Debug.Log("ValidateBlocks with robot.knownObjects.Count("+robot.knownObjects.Count+")");

		//loop through our 'ideal' layout blocks and look for known objects that might satisfy the requirements of each
		for(int layoutBlockIndex=0; layoutBlockIndex<layout.blocks.Count; layoutBlockIndex++) {
			BuildInstructionsCube block = layout.blocks[layoutBlockIndex];

			//double the lenience for an object that previously satisfied this block
			ObservedObject previouslyAssigned = null;
			if(block.Validated) {
				previouslyAssigned = block.AssignedObject;
			}
			block.Validated = false;
			block.AssignedObject = null;

			if(debug) Debug.Log("attempting to validate block("+block.gameObject.name+") of type("+block.cubeType+")");
			
			//search through known objects for one that can be assigned
			for(int objectIndex=0; objectIndex<potentialObservedObjects.Count; objectIndex++) {
				
				ObservedObject newObject = potentialObservedObjects[objectIndex];
				float extraFudgeFactor = 1f;
				if(previouslyAssigned == newObject) {
					extraFudgeFactor = 2f;
				}
				if(debug) Debug.Log("checking if knownObject("+newObject+"):index("+objectIndex+"):cubeType("+newObject.cubeType+") can satisfy layoutCube("+block.gameObject.name+")");
				
				//cannot validate block in hand
				if(!block.isHeld && newObject == robot.carryingObject) {
					if(debug) Debug.Log("cannot validate layout cube with carryingObject("+robot.carryingObject+")");
					continue;
				}
				
				//skip objects of the wrong type
				if(!block.SatisfiedByObject(newObject, distanceFudge*extraFudgeFactor, coplanarFudge*extraFudgeFactor, angleFudge, true, debug) ) {
					if(debug) Debug.Log("skip object("+CozmoPalette.instance.GetNameForObjectType(newObject.cubeType)+") because it doesn't satisfy layoutCube("+block.gameObject.name+")");
					continue;
				}

				SetBlockValidated(layoutBlockIndex, newObject);
				break;
			}
			
		}
		
		validCount = validated.Count;
		
		if( validCount > lastValidCount && validCount < layout.blocks.Count ) {
			AudioManager.PlayAudioClip(cubePlaced, 0, AudioManager.Source.Notification);
		}
	}

	void SetBlockValidated(int layoutBlockIndex, ObservedObject newObject) {
		BuildInstructionsCube block = currentLayout.blocks[layoutBlockIndex];
		validated.Add(block);
		block.AssignedObject = newObject;
		block.Validated = true;
		block.Highlighted = false;
		potentialObservedObjects.Remove(newObject);

		if(newObject.isActive) {
			ActiveBlock activeBlock = newObject as ActiveBlock;
			SetLightCubeToCorrectColor(activeBlock, block);
		}
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
		ghostBlock.cubeType = failingLayoutBlock.cubeType;
		ghostBlock.activeBlockMode = failingLayoutBlock.activeBlockMode;
		ghostBlock.baseColor = failingLayoutBlock.baseColor;
		ghostBlock.Hidden = true;
		ghostBlock.Highlighted = true;

		//Debug.Log("ghostBlock type("+ghostBlock.objectType+") family("+ghostBlock.objectFamily+") baseColor("+ghostBlock.baseColor+")");
	}

	ObservedObject GetKnownObjectForInventorySlot(BuildInstructionsCube block, int dupe) {
		if(robot == null) return null;
		
		int count = 0;
		
		//Debug.Log("GetKnownObjectForInventorySlot robot.knownObjects.Count("+robot.knownObjects.Count+")");
		
		for(int i=0;i<robot.knownObjects.Count;i++) {
			
			ObservedObject obj = robot.knownObjects[i];
			
			if(obj.cubeType != block.cubeType) continue;
			
			count++;
			if(count == dupe) return obj;
		}
		
		return null;
	}

	int GetKnownObjectCountForBlock(BuildInstructionsCube block) {
		if(robot == null) return 0;

		int count = 0;

		//Debug.Log("GetKnownObjectCountForBlock robot.knownObjects.Count("+robot.knownObjects.Count+")");

		for(int i=0;i<robot.knownObjects.Count;i++) {

			ObservedObject obj = robot.knownObjects[i];

			if(obj.cubeType != block.cubeType) continue;

			count++;
		}

		return count;
	}

	ObservedObject GetObservedObjectForNextLayoutBlock() {
		BuildInstructionsCube layoutCube = currentLayout.blocks.Find ( x => !x.Validated );
		
		if (layoutCube == null) return null;
		
		unplacedObjects.Clear();
		unplacedObjects.AddRange (robot.knownObjects);
		
		if(unplacedObjects.Count == 0) return null;

		for (int i=0; i<currentLayout.blocks.Count; i++) {
			if(!currentLayout.blocks[i].Validated) continue;
			unplacedObjects.Remove (currentLayout.blocks[i].AssignedObject);
		}
		
		//if our carried object satisfies our needs, just use it
		ObservedObject obj = (unplacedObjects.Contains(robot.carryingObject) && IsMatchingBlock(layoutCube, robot.carryingObject, true)) ? robot.carryingObject : null;

		if(obj == null) {
			//or find closest object that matches our needs
			float closest = float.MaxValue;
			for(int i=0;i<unplacedObjects.Count;i++) {
				if(!IsMatchingBlock(layoutCube, unplacedObjects[i], true)) continue;
				float dist = (unplacedObjects[i].WorldPosition - robot.WorldPosition).magnitude;
				if(dist > closest) continue;
				closest = dist;
				obj = unplacedObjects[i];
			}
		}

		return obj;
	}
	
	//we are dropping an object and want to know which blocks are already on the ground and valid
	//	so we can then do distance checks if necessaril to validate the placement of the drop
	bool IsValidGroundBlock(BuildInstructionsCube block) {
		if(!block.Validated) return false;
		if(block.cubeBelow != null) return false;
		
		return true;
	}
	
	bool IsMatchingBlock(BuildInstructionsCube block, ObservedObject objectToMatch, bool ignoreColorOverride=false) {
		if(block.cubeType != objectToMatch.cubeType) return false;
		if(!ignoreActiveColor && !ignoreColorOverride && block.isActive && robot.activeBlocks[objectToMatch].mode != block.activeBlockMode) return false;
		return true;
	}
	
	//we are dropping an object and want to know which layout block it might be an apt match for
	bool IsUnvalidatedMatchingGroundBlock(BuildInstructionsCube block, ObservedObject objectToMatch, bool ignoreColorOverride=false) {
		if(block.Validated) return false;
		if(block.cubeBelow != null) return false;
		if(block.cubeType != objectToMatch.cubeType) return false;
		if(!ignoreActiveColor && !ignoreColorOverride && block.isActive && robot.activeBlocks[objectToMatch].mode != block.activeBlockMode) return false;
		return true;
	}
	
	bool IsUnvalidatedMatchingStackedBlock(BuildInstructionsCube block, ObservedObject objectToMatch, bool ignoreColorOverride=false) {
		if(block.Validated) return false;
		if(block.cubeBelow == null) return false;
		if(block.cubeType != objectToMatch.cubeType) return false;
		if(!ignoreActiveColor && !ignoreColorOverride && block.isActive && robot.activeBlocks[objectToMatch].mode != block.activeBlockMode) return false;
		return true;
	}
	
	bool IsActiveBlockCorrectColor(ActiveBlock activeBlockToMatch) {
		
		for(int i=0; i<currentLayout.blocks.Count; i++) {
			BuildInstructionsCube block = currentLayout.blocks[i];
			if(block.Validated) continue;
			if(!block.isActive) continue;
			if(activeBlockToMatch.mode == block.activeBlockMode) return true;
		}
		
		return false;
	}
	
	//allow us to predetermine the target layout cube if we want to, if not we'll reference the first one we find
	void SetLightCubeToCorrectColor(ActiveBlock activeBlock, BuildInstructionsCube layoutActiveCube=null) {
		if(layoutActiveCube  == null) {
			layoutActiveCube = currentLayout.blocks.Find (x => !x.Validated && x.isActive);
			if(layoutActiveCube == null) return;
		}
		if(layoutActiveCube.activeBlockMode == activeBlock.mode) return;
		if(CozmoBusyPanel.instance != null) {
			string desc = null;
			CozmoBusyPanel.instance.SetDescription("change mode of\n", activeBlock, ref desc);
		}
		//robot.localBusyTimer = (cycleTime * cycleCount) + cycleDelayTime;
		StartCoroutine(CycleLightCubeModes(activeBlock, layoutActiveCube.activeBlockMode));
	}
	
	IEnumerator CycleLightCubeModes(ActiveBlock activeBlock, ActiveBlock.Mode mode) {
		yield return new WaitForSeconds(cycleDelayTime);
		
		int startingIndex = (int)mode - cycleCount;
		if(startingIndex < 0) startingIndex += ActiveBlock.Mode.Count;
		
		activeBlock.SetMode((ActiveBlock.Mode)startingIndex);
		
		while(activeBlock.mode != mode) {
			AudioManager.PlayOneShot(cycleSound);
			activeBlock.CycleMode();
			
			yield return new WaitForSeconds(cycleTime);
		}
		
		AudioManager.PlayOneShot(cycleSuccessSound);
		//robot.isBusy = false;
	}

	#endregion

	#region PUBLIC METHODS
	
	public void Show() {
		hidden = false;
	}
	
	public void Hide() {
		hidden = true;
		layoutInstructionsPanel.SetActive(false);
		inventoryPanel.SetActive(false);
		layoutInstructionsCamera.gameObject.SetActive(false);
	}

	public void DebugQuickValidate() {
		ignoreActiveColor = true;
		AnalyzeLayoutForValidation();
		StartGame();

		Debug.Log("DebugQuickValidate validated("+validated.Count+")");
	}

	public void TryToValidate() {
		AnalyzeLayoutForValidation();
	}

	public void StartGame() {
		iStartGame = true;
	}

	public void StartBuild() {
		iStartBuild = true;
	}

	public void StartAutoBuild() {
		iStartAutoBuild = true;
	}

	public Vector3 GetPoseFromLayoutForTransform(Transform t, out float facingAngle, out Vector3 facingVector, Vector3 directionOverride) {
		facingAngle = 0f;
		facingVector = Vector3.zero;
		
		if(currentLayout == null) return Vector3.zero;
		if(t == null) return Vector3.zero;
		
		facingVector = CozmoUtil.Vector3UnityToCozmoSpace(t.forward);
		facingAngle = MathUtil.SignedVectorAngle(Vector3.right, facingVector, Vector3.forward) * Mathf.Deg2Rad;
		
		Vector3 pose = (CozmoUtil.Vector3UnityToCozmoSpace(t.position) / currentLayout.scale) * CozmoUtil.BLOCK_LENGTH_MM;
		return pose;
	}
	

	public Vector3 GetStartingPositionFromLayout(out float facingAngle, out Vector3 facingVector) {
		facingAngle = 0f;
		facingVector = Vector3.zero;

		if(currentLayout == null) return Vector3.zero;
		if(currentLayout.startPositionMarker == null) return Vector3.zero;

		return GetPoseFromLayoutForTransform(currentLayout.startPositionMarker, out facingAngle, out facingVector, Vector3.zero);
	}

	public Vector3 GetPoseFromLayoutForTransformOld(Transform t, out float facingAngle, out Vector3 facingVector, Vector3 directionOverride) {
		facingAngle = 0f;
		facingVector = Vector3.zero;
		
		if(currentLayout == null) return Vector3.zero;
		if(t == null) return Vector3.zero;
		
		List<BuildInstructionsCube> layoutBlocksOnGround = currentLayout.blocks.FindAll(x => x.cubeBelow == null && x.Validated);

		if(layoutBlocksOnGround.Count == 0)
		{
			Debug.LogWarning( "layoutBlocksOnGround's count is 0" );
			return Vector3.zero;
		}

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

	public bool AttemptAssistedPlacement() {
		ObservedObject objectToPlace = robot.carryingObject;
		Vector3 pos = Vector3.zero;
		float facing_rad = 0f;
		
		AnalyzeLayoutForValidation ();
		
		if(robot == null) return false;
		if(objectToPlace == null) return false;
		
		List<BuildInstructionsCube> newBlocks = currentLayout.blocks.FindAll(x => IsUnvalidatedMatchingGroundBlock(x, objectToPlace, true));
		
		if(newBlocks == null || newBlocks.Count == 0) {
			//this is probably ok?  may need to do more processing to see if its ok
			
			newBlocks = currentLayout.blocks.FindAll(x => IsUnvalidatedMatchingStackedBlock(x, objectToPlace, true));
			if(newBlocks == null || newBlocks.Count == 0) {
				Debug.Log ("This block is not required in this layout.");
				return false;
			}
			else {
				return AttemptAssistedStack(objectToPlace, newBlocks);
			}
		}

		BuildInstructionsCube bestBlock = newBlocks[0];
		if(newBlocks.Count > 1) {
			float closest = float.MaxValue;
			for(int i=0;i<newBlocks.Count;i++) {

				Vector2 robotPos = robot.WorldPosition;
				Vector2 blockPos = newBlocks[i].WorldPosition;

				float range = (blockPos - robotPos).sqrMagnitude;
				if(range < closest) {
					closest = range;
					bestBlock = newBlocks[i];
				}
			}
		}	

		if (bestBlock.isActive) {
			ActiveBlock activeBlock = objectToPlace as ActiveBlock;
			SetLightCubeToCorrectColor(activeBlock);
		}
		
		pos = bestBlock.GetCozmoSpacePose(out facing_rad); 
		string description = null;
		CozmoBusyPanel.instance.SetDescription( "drop\n", robot.carryingObject, ref description );
		robot.DropObjectAtPose(pos, facing_rad);
		
		return true;
	}

	public bool AttemptAssistedStack( ObservedObject objectToStack, List<BuildInstructionsCube> potentiallyStackable)  {

		List<BuildInstructionsCube> potentiallyStackedUpon = currentLayout.blocks.FindAll (x => x.cubeAbove != null && !x.cubeAbove.Validated); //newBlocks.Find ( y => y.cubeBelow == x ) != null );
		
		if(potentiallyStackedUpon == null || potentiallyStackedUpon.Count == 0) {
			//if this will be our first ground block to validate, automatically valid location
			Debug.Log ("No prior Blocks.");
			return false;
		}

		float closestRange = float.MaxValue;

		BuildInstructionsCube layoutBlockToStack = potentiallyStackable [0];
		ObservedObject objectToStackUpon = potentiallyStackedUpon [0].AssignedObject;
		if(objectToStackUpon == null) {
			objectToStackUpon = robot.knownObjects.Find ( x => IsUnvalidatedMatchingGroundBlock(potentiallyStackedUpon [0], x, true) );
		}

		for(int i=0;i<potentiallyStackedUpon.Count;i++) {
			ObservedObject obj = potentiallyStackedUpon[i].AssignedObject;
			if(obj == null) {
				obj = robot.knownObjects.Find ( x => IsUnvalidatedMatchingGroundBlock(potentiallyStackedUpon[i], x, true) );
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
			SetLightCubeToCorrectColor(activeBlock, layoutBlockToStack);
		}


		if(CozmoBusyPanel.instance != null)	{
			
			if(objectToStack != null) {
				string desc = "Cozmo is attempting to stack\n";
				
				if(objectToStack.isActive) {
					desc += "an Active Block";
				}
				else {
					desc += "a ";
					
					if(CozmoPalette.instance != null) {
						desc += CozmoPalette.instance.GetNameForObjectType(objectToStack.cubeType) + " ";
					}
				}
				
				if(objectToStackUpon != null) {
					
					desc += "\n on top of ";
					
					if(objectToStackUpon.isActive) {
						desc += "an Active Block";
					}
					else {
						desc += "a ";
						
						if(CozmoPalette.instance != null) {
							desc += CozmoPalette.instance.GetNameForObjectType(objectToStackUpon.cubeType) + " ";
						}
					}
				}
				
				desc += ".";
				
				CozmoBusyPanel.instance.SetDescription(desc);
			}
		}

		robot.PickAndPlaceObject( objectToStackUpon );

		return true;
	}

	public bool PredictDropValidation( ObservedObject objectToDrop, out string errorText, out LayoutErrorType errorType, out bool approveStandardDrop) {
		errorText = "";
		errorType = LayoutErrorType.NONE;
		approveStandardDrop = false;
		
		if(robot == null) return false;
		if(objectToDrop == null) return false;
		
		Vector3 posToDrop = robot.WorldPosition + robot.Forward * CozmoUtil.BLOCK_LENGTH_MM + Vector3.forward * CozmoUtil.BLOCK_LENGTH_MM * 0.5f;
		
		List<BuildInstructionsCube> newBlocks = currentLayout.blocks.FindAll(x => IsUnvalidatedMatchingGroundBlock(x, objectToDrop));
		
		if(newBlocks == null || newBlocks.Count == 0) {
			//this is probably ok?  may need to do more processing to see if its ok
			//error = "No block of this type should be placed here.";
			approveStandardDrop = true;
			return false;
		}

		//see if this drop object will satisfy any layout blocks, set angleFudge to 180f to ignore angle for this prediction (our assisted placement logic will enforce rotation
		for(int newIndex=0; newIndex < newBlocks.Count; newIndex++) {
			BuildInstructionsCube block = newBlocks[newIndex];
			if(!block.PredictSatisfaction(objectToDrop, posToDrop, robot.Rotation, distanceFudge * 2f, float.MaxValue, 180f, true)) continue;
			return true;
		}
		
		return false;
	}

	public bool PredictStackValidation( ObservedObject objectToStack, ObservedObject objectToStackUpon, out string errorText, out LayoutErrorType errorType, bool ignoreColor) {

		errorText = "";
		errorType = LayoutErrorType.NONE;

		BuildInstructionsCube layoutBlockToStackUpon = currentLayout.blocks.Find(x => x.AssignedObject == objectToStackUpon);
		if(layoutBlockToStackUpon == null) {
			layoutBlockToStackUpon = currentLayout.blocks.Find(x => IsUnvalidatedMatchingGroundBlock(x, objectToStackUpon, ignoreColor) );
		
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

		if(layoutBlockToStack.cubeType != objectToStack.cubeType) {
			errorText = "You are attempting to stack the wrong type of cube.";
			errorType = LayoutErrorType.WRONG_BLOCK;
			return false;
		}

		if(layoutBlockToStack.isActive) {
			if(!ignoreColor && !ignoreActiveColor && robot.activeBlocks[objectToStack].mode != layoutBlockToStack.activeBlockMode) {
				errorText = "This active block needs to be "+layoutBlockToStack.activeBlockMode+" before it is stacked.";
				errorType = LayoutErrorType.WRONG_COLOR;
				return false;
			}
		}

		return true;
	}

	public void SetMessage(string message, Color color) {

		screenMessage.ShowMessageForDuration(message, 5f, color);
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

	#endregion

}
