using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class SlalomController : GameController {
	public static SlalomController instance = null;

	[SerializeField] Text text_finalTime;
	[SerializeField] Text text_resultsTitle;
	[SerializeField] Button button_PlayAgain;
	[SerializeField] Button button_Rebuild;

	[SerializeField] bool thereAndBackAgain = true;	//when reaching the end or beginning of obstacle list, reverse order
	[SerializeField] int pointsPerObstacle = 100;
	[SerializeField] Text textObservedCount = null;
	[SerializeField] Text textProgress = null;

	[SerializeField] AudioClip cornerTriggeredSound = null;
	[SerializeField] AudioClip finalLapSound = null;
	[SerializeField] AudioClip[] lapsRemainingSound = null;
	[SerializeField] ActiveBlock.Mode nextColor;
	[SerializeField] ActiveBlock.Mode currentColor;

	[SerializeField] float previewCourseLightsDelay = 0.1f;
	[SerializeField] float previewCourseLightsPause = 1f;
	[SerializeField] float previewCourseLightsTimer = 0f;

	[SerializeField] int[] startingCornerIndexPerLevel;

	int startingCornerIndex = 0;

	List<ActiveBlock> obstacles = new List<ActiveBlock>();

	bool atYourMark = false;
	bool activeBlockMovedorDeleted = false;

	int currentCorner = 0;
	int currentObstacleIndex = 0;
	bool clockwise = false;
	bool forward = true;

	uint nextColor_uint;
	uint currentColor_unit;

	int currentLap = 1;
	float timeOfLastCorner = 0f;
	Vector3 startPosition = Vector3.zero;
	Vector3 startFacing = Vector3.right;
	bool wasAtMark = false;
	int lastLapCount = 1;
	public int testLightIndex = 0;

	readonly float[] idealCornerAngles = { 45f, 135f, 225f, 315f };

	float cornerTriggeredDelay { get { return cornerTriggeredSound != null ? cornerTriggeredSound.length : 0f; } }

	ActiveBlock previousObstacle = null;

	ActiveBlock currentObstacle { 
		get {
			if(obstacles == null) return null;
			if(currentObstacleIndex < 0) return null;
			if(currentObstacleIndex >= obstacles.Count) return null;

			return obstacles[currentObstacleIndex];
		}
	}
	ActiveBlock nextObstacle { 
		get { 
			if(obstacles == null) return null;

			int nextIndex = GetNextIndex();

			if(nextIndex < 0) return null;
			if(nextIndex >= obstacles.Count) return null;

			return obstacles[nextIndex];
		}
	}

	int GetNextIndex(bool actual=false) {

		//if(actual) Debug.Log("start GetNextIndex("+actual+") forward("+forward+") currentObstacleIndex("+currentObstacleIndex+") currentLap("+currentLap+")");

		int nextIndex = 0;
		if(forward) {
			nextIndex = currentObstacleIndex + 1;
			if(nextIndex >= obstacles.Count) {
				if(thereAndBackAgain) {
					if(actual) forward = false;
					nextIndex = obstacles.Count - 2;
				}
				else {//loop
					nextIndex = 0;
				}

				if(actual) {
					currentLap++;
					//Debug.Log("currentLap("+currentLap+")");
				}
			}
		}
		else {
			nextIndex = currentObstacleIndex - 1;
			if(nextIndex < 0) {
				if(thereAndBackAgain) {
					nextIndex = 1;
					if(actual) {
						forward = true;
					}
				}
				else {//loop
					nextIndex = obstacles.Count - 1;
				}
			}
		}

		//if(actual) Debug.Log("finish GetNextIndex("+actual+") forward("+forward+") nextIndex("+nextIndex+") currentLap("+currentLap+")");

		return nextIndex;
	}

	void LapComplete() {
		//score += pointsPerObstacle;
		//Debug.Log("LapComplete scores[0](" + scores[0].ToString() + ")");
		//PlayOneShot(playerScoreSound);
		int lapsRemaining = (int)((float)levelData.scoreToWin / ((float)pointsPerObstacle * obstacles.Count)) - currentLap;
		if(lapsRemaining == 0) {
			if(finalLapSound != null) AudioManager.PlayAudioClip(finalLapSound, cornerTriggeredDelay, AudioManager.Source.Notification);
			Debug.Log("final lap");
		}
		else {
			if(lapsRemainingSound.Length > 0 && lapsRemaining > 0 && lapsRemaining < timerSounds.Length && timerSounds[lapsRemaining].sound != null) {
				lapsRemainingSound[0] = timerSounds[lapsRemaining].sound;
				AudioManager.PlayAudioClips(lapsRemainingSound, cornerTriggeredDelay, 0.05f, false, false, AudioManager.Source.Notification);
			}
			Debug.Log((lapsRemaining + 1) + " laps remaining");
		}
	}

	int GetPreviousIndex() {
		int nextIndex = 0;
		if(!forward) {
			nextIndex = currentObstacleIndex + 1;
			if(nextIndex >= obstacles.Count) {
				if(thereAndBackAgain) {
					nextIndex = obstacles.Count - 2;
				}
				else {//loop
					nextIndex = 0;
				}
			}
		}
		else {
			nextIndex = currentObstacleIndex - 1;
			if(nextIndex < 0) {
				if(thereAndBackAgain) {
					nextIndex = 1;
				}
				else {//loop
					nextIndex = obstacles.Count - 1;
				}
			}
		}
		
		return nextIndex;
	}

	protected override void OnEnable()
	{
		base.OnEnable();

		instance = this;

		if(CozmoPalette.instance != null) {
			nextColor_uint = CozmoPalette.instance.GetUIntColorForActiveBlockType(nextColor);
			currentColor_unit = CozmoPalette.instance.GetUIntColorForActiveBlockType(currentColor);
		}
		MessageDelay = 0f;

		startingCornerIndex = 0;
		if(startingCornerIndexPerLevel != null && currentLevelNumber >= startingCornerIndexPerLevel.Length) {
			startingCornerIndex = startingCornerIndexPerLevel[currentLevelNumber-1];
		}
	}
	
	protected override void OnDisable()
	{
		base.OnDisable();
		
		if( instance == this ) instance = null;
	}

	protected override void Update_BUILDING() {
		base.Update_BUILDING();

		if(textObservedCount != null) {
			textObservedCount.text = "obstacles: " +obstacles.Count.ToString();
		}
	}

	protected override void Enter_PRE_GAME() {
		base.Enter_PRE_GAME();

		GotoStartPose();
		PrepareGameForPlay(startPosition, startFacing, true);
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure += CheckForGotoPoseCompletion;
		previewCourseLightsTimer = previewCourseLightsDelay;
	}

	bool goingToFinalGoal = true;
	void GotoStartPose() {
		float rad;
		startPosition = GameLayoutTracker.instance.GetStartingPositionFromLayout(out rad, out startFacing);
		Vector3 robotToStart = startPosition - robot.WorldPosition;
		if(robotToStart.magnitude > CozmoUtil.BLOCK_LENGTH_MM * 4f) {
			Vector3 halfWayPoint = robot.WorldPosition + robotToStart.normalized * CozmoUtil.BLOCK_LENGTH_MM * 4f;
			halfWayPoint = robot.NudgePositionOutOfObjects(halfWayPoint);
			rad = MathUtil.SignedVectorAngle(Vector3.right, robotToStart.normalized, Vector3.forward) * Mathf.Deg2Rad;
			robot.GotoPose(halfWayPoint.x, halfWayPoint.y, rad);
			goingToFinalGoal = false;
		}
		else {
			goingToFinalGoal = true;
			robot.GotoPose(startPosition.x, startPosition.y, rad);
		}
		CozmoBusyPanel.instance.SetDescription("Cozmo is getting in the starting position.");
		atYourMark = false;
		wasAtMark = false;
		robot.isBusy = true;
	}

	void CheckForGotoPoseCompletion(bool success, RobotActionType action_type) {

		switch(action_type) {
			case RobotActionType.DRIVE_TO_POSE:
				if(success && goingToFinalGoal) {
					atYourMark = true;
					if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure -= CheckForGotoPoseCompletion;
				}
				else { //try again or fail outright?
					GotoStartPose();
				}
				break;
		}
	}

	protected override void Update_PRE_GAME() {
		//only let our countdown start when our goto command is done
		//todo make it have to succeed?

		if (!atYourMark) {
			
			if (previewCourseLightsTimer > 0f) {
				previewCourseLightsTimer -= Time.deltaTime;
				if (previewCourseLightsTimer <= 0f) {
					AdvanceCorner(true);
					previewCourseLightsTimer = (currentObstacleIndex == 0 && currentCorner == 0) ? previewCourseLightsPause : previewCourseLightsDelay;
				}
			}
			return;
		}

		if (!wasAtMark) {
			PrepareGameForPlay(startPosition, startFacing);
		}

		wasAtMark = true;


		base.Update_PRE_GAME();
	}

	protected override void Exit_PRE_GAME() {
		robot.isBusy = false;
		base.Exit_PRE_GAME();
	}

	void PrepareGameForPlay(Vector3 startingPos, Vector3 startingFacing, bool preview=false) {

		obstacles.Clear();
		List<ObservedObject> observedObjects = GameLayoutTracker.instance.GetTrackedObjectsInOrder().FindAll( x => x.isActive );
		for(int i = 0; i < observedObjects.Count; ++i) obstacles.Add(observedObjects[i] as ActiveBlock);

		lastLapCount = 1;
		currentLap = 1;
		currentObstacleIndex = 0;

		if(thereAndBackAgain) {
			previousObstacle = nextObstacle;
		}
		else {
			int index = currentObstacleIndex - 1;
			if(index < 0) index = obstacles.Count - 1;
			previousObstacle = obstacles[index];
		}

		if(currentObstacle == null) {
			Debug.LogError("currentObstacle is null");
			return;
		}

		//Debug.Log("PrepareGameForPlay currentCorner("+currentCorner+") nextCorner("+nextCorner+") onNextObstacle("+onNextObstacle+")");

		Vector2 startToFirst = currentObstacle.WorldPosition - startingPos;
		Vector3 cross = Vector3.Cross(startingFacing, Vector3.forward);
		
		clockwise = Vector3.Dot(startToFirst, cross) > 0f;

//		//shift this up so we can see better
//		startingPos += Vector3.forward * CozmoUtil.BLOCK_LENGTH_MM;
//
//		Vector3 facingLocation = startingPos + startingFacing * CozmoUtil.BLOCK_LENGTH_MM * 10f;
//		RobotEngineManager.instance.VisualizeQuad(11, CozmoPalette.ColorToUInt(Color.blue), startingPos, startingPos, facingLocation, facingLocation);
//
//		Vector3 crossLocation = startingPos + (Vector3)cross * CozmoUtil.BLOCK_LENGTH_MM * 10f;
//
//		RobotEngineManager.instance.VisualizeQuad(12, CozmoPalette.ColorToUInt(Color.green), startingPos, startingPos, crossLocation, crossLocation);
//
//		RobotEngineManager.instance.VisualizeQuad(13, CozmoPalette.ColorToUInt(Color.yellow), startingPos, startingPos, currentObstacle.WorldPosition, currentObstacle.WorldPosition);

		currentCorner = startingCornerIndex;
		bool onNextObstacle;
		int nextCorner = GetNextCorner(out onNextObstacle);
		UpdateCornerLights(currentCorner, nextCorner, onNextObstacle, preview);
	}

	protected override void Enter_PLAYING() {
		base.Enter_PLAYING();

		activeBlockMovedorDeleted = false;
		timeOfLastCorner = Time.time;

		for(int i = 0; i < obstacles.Count; ++i) {
			obstacles[i].OnAxisChange += OnActiveBlockRolled;
			obstacles[i].OnDelete += OnActiveBlockDeleted;
		}
	}

	protected override void Update_PLAYING() {
		base.Update_PLAYING();

		if(currentObstacle == null) return;

		if(Time.time > timeOfLastCorner + 10) {
			AudioManager.PlayAudioClip(instructionsSound, 0f, AudioManager.Source.Notification);
			timeOfLastCorner = Time.time;
		}

		Vector2 robotPos = robot.WorldPosition;

		Vector2 obstacleToRobotPos = robotPos - (Vector2)currentObstacle.WorldPosition;

		//TestLights();

		Vector3 cornerVector = GetCornerVector(currentObstacle, currentCorner, clockwise, previousObstacle);
		float newAngle = Vector3.Angle(cornerVector, obstacleToRobotPos.normalized);

		if( newAngle < 10f ) {
			//Debug.Log("Update_PLAYING AdvanceCorner because clockwise("+clockwise+") newAngle("+newAngle+") cornerVector("+cornerVector+")");
			AdvanceCorner();
		}

		if(textObservedCount != null) {
			textObservedCount.text = "obstacles: " +obstacles.Count.ToString();
		}

		if(textProgress != null) {
			textProgress.text = "index("+currentObstacleIndex+") corner("+currentCorner+")";
		}
	}

	protected override void Exit_PLAYING(bool overrideStars = false) {
		base.Exit_PLAYING(true);

		stars = 0;
		int[] starRequirements = levelData.stars;
		for(int i = 0; i < starRequirements.Length; ++i) {
			if(stateTimer <= starRequirements[i] && starRequirements[i] > 0) stars = i + 1;
		}

		if(stars >= savedStars) savedStars = stars;

		for(int i = 0; i < obstacles.Count; ++i) {
			obstacles[i].OnAxisChange -= OnActiveBlockRolled;
			obstacles[i].OnDelete -= OnActiveBlockDeleted;
		}

		text_finalTime.text = "Final Time: " + stateTimer.ToString("f2") + " seconds";

		if(robot != null) robot.TurnOffAllLights();
	}

	protected override bool IsGameOver() {
		//game specific end conditions...
		if(activeBlockMovedorDeleted) return true;

		return base.IsGameOver();
	}

	private void OnActiveBlockRolled(ActiveBlock activeBlock)
	{
		activeBlockMovedorDeleted = true;
	}

	private void OnActiveBlockDeleted(ObservedObject observedObject)
	{
		OnActiveBlockRolled(observedObject as ActiveBlock);
	}

	private Vector2 GetCornerVector(ActiveBlock obstacle, int cornerIndex, bool clock, ActiveBlock previous, bool debug=false) {

		float angle = idealCornerAngles[cornerIndex] * (clock ? -1f : 1f);

		Vector2 corner = Vector3.up;

		Vector2 currentToLast = previous.WorldPosition - obstacle.WorldPosition;

		Vector2 idealCorner = Quaternion.AngleAxis(angle, Vector3.forward) * currentToLast.normalized;

		Vector2[] corners = { obstacle.TopNorthEast, obstacle.TopSouthEast, obstacle.TopSouthWest, obstacle.TopNorthWest };

		float bestDot = -float.MaxValue;

		for(int i=0; i<4; i++) {
			float dot = Vector2.Dot(idealCorner, corners[i]);
			if(dot > bestDot) {
				bestDot = dot;
				corner = corners[i];
			}
		}

		if(debug) Debug.Log("GetCornerVector obstacle("+obstacle+") cornerIndex("+cornerIndex+") clock("+clock+") previous("+previous+") angle("+angle+") idealCorner("+idealCorner+") corner("+corner+") currentToLast("+currentToLast.normalized+")");

		return corner;
	}



	private void AdvanceCorner(bool preview=false) {

		//trigger lap score and sound when we cross our first corner again
		if(!preview && currentCorner == 0 && lastLapCount != currentLap) {
			LapComplete();
		}

		lastLapCount = currentLap;

		bool obstacleAdvanced;
		currentCorner = GetNextCorner(out obstacleAdvanced);

		if(obstacleAdvanced) {
			score += pointsPerObstacle;
			previousObstacle = currentObstacle;
			currentObstacleIndex = GetNextIndex(true);
			clockwise = !clockwise;
		}

		bool nextCornerIsOnNextObstacle;
		int nextCorner = GetNextCorner(out nextCornerIsOnNextObstacle);

		if(!preview) AudioManager.PlayOneShot(cornerTriggeredSound, 0f, currentCorner == 0 ? 4 : currentCorner);

		timeOfLastCorner = Time.time;

		//Debug.Log("AdvanceCorner obstacleAdvanced("+obstacleAdvanced+") clockwise("+clockwise+") currentCorner("+currentCorner+") nextCorner("+nextCorner+") nextCornerIsOnNextObstacle("+nextCornerIsOnNextObstacle+")");

		UpdateCornerLights(currentCorner, nextCorner, nextCornerIsOnNextObstacle, preview);
		if(!preview) UpdateRobotLights(currentCorner);
	}

	int GetNextCorner(out bool onNextObstacle) {
		onNextObstacle = false;
		if(currentObstacle == null) return 0;
		if(nextObstacle == null) return 0;

		Vector2 currentToLast = previousObstacle.WorldPosition - currentObstacle.WorldPosition;
		Vector2 currentToNext = nextObstacle.WorldPosition - currentObstacle.WorldPosition;
		
		float totalAngleToTraverseOnThisLeg = Vector3.Angle(currentToLast, currentToNext);
		if(totalAngleToTraverseOnThisLeg < 90f) totalAngleToTraverseOnThisLeg = 360f - totalAngleToTraverseOnThisLeg;

		int corner = currentCorner+1;

		if(corner > 3 || idealCornerAngles[corner] > totalAngleToTraverseOnThisLeg) {
			onNextObstacle = true;
			corner = 0;
		}

		return corner;
	}

	private void UpdateRobotLights(int corner)
	{
		if(corner == 0) {
			for(int i = 0; i < robot.lights.Length; ++i) {
				robot.lights[i].onColor = nextColor_uint;
				robot.lights[i].offColor = 0;
				robot.lights[i].onPeriod_ms = 500;
				robot.lights[i].offPeriod_ms = Robot.Light.FOREVER;
			}
			for(int i = 0; i < previousObstacle.lights.Length; ++i) {
				nextObstacle.lights[i].onColor = nextColor_uint;
				nextObstacle.lights[i].offColor = 0;
				nextObstacle.lights[i].onPeriod_ms = 500;
				nextObstacle.lights[i].offPeriod_ms = ActiveBlock.Light.FOREVER;
			}
		}
		else {
			for(int i = 0; i < robot.lights.Length; ++i) {
				if(i < corner) {
					robot.lights[i].onColor = currentColor_unit;
				}
				else {
					robot.lights[i].onColor = 0;
				}
				robot.lights[i].onPeriod_ms = Robot.Light.FOREVER;
				robot.lights[i].offPeriod_ms = 0;
			}
		}
	}

	private void UpdateCornerLights(int corner, int next, bool nextCornerOnNextObstacle, bool preview=false, bool debug=false)
	{
		if(robot == null) return;
		if(currentObstacle == null) return;
		if(nextObstacle == null) return;

		Vector2 cornerVector = GetCornerVector(currentObstacle, corner, clockwise, previousObstacle, debug);
		Vector2 nextCornerVector = Vector3.zero;

		if(nextCornerOnNextObstacle) {
			nextCornerVector = GetCornerVector(nextObstacle, next, !clockwise, currentObstacle, debug);
		}
		else {
			nextCornerVector = GetCornerVector(currentObstacle, next, clockwise, previousObstacle, debug);
		}

		for(int obstacleIndex=0; obstacleIndex < obstacles.Count; obstacleIndex++) {
			ActiveBlock obstacle = obstacles[obstacleIndex];
			obstacle.relativeMode = 0;

			int currentTopLightIndex = -1;
			int currentBottomLightIndex = -1;
			int nextTopLightIndex = -1;
			int nextTopBottomIndex = -1;

			Vector2 obstacleNorth = obstacle.TopNorth;

			if(obstacle == currentObstacle) {
				float angleFromNorth = MathUtil.SignedVectorAngle(obstacleNorth, cornerVector, Vector3.forward);
				currentTopLightIndex = ActiveBlock.Light.GetIndexForCornerClosestToAngle(angleFromNorth, true);
				currentBottomLightIndex = ActiveBlock.Light.GetIndexForCornerClosestToAngle(angleFromNorth, false);
				if(debug) Debug.Log("obstacle("+obstacle+") angleFromNorth("+angleFromNorth+") index("+currentTopLightIndex+") currentObstacle("+currentObstacle+") nextObstacle("+nextObstacle+")");
			}

			if(obstacle == (nextCornerOnNextObstacle ? nextObstacle : currentObstacle)) {
				float angleFromNorth = MathUtil.SignedVectorAngle(obstacleNorth, nextCornerVector, Vector3.forward);
				nextTopLightIndex = ActiveBlock.Light.GetIndexForCornerClosestToAngle(angleFromNorth, true);
				nextTopBottomIndex = ActiveBlock.Light.GetIndexForCornerClosestToAngle(angleFromNorth, false);

				if(debug) Debug.Log("obstacle("+obstacle+") angleFromNorth("+angleFromNorth+") index("+nextTopLightIndex+") currentObstacle("+currentObstacle+") nextObstacle("+nextObstacle+")");
			}

			//refresh light settings
			for(int i = 0; i < obstacle.lights.Length; ++i) {

				if(i == currentTopLightIndex || i == currentBottomLightIndex) {
					obstacle.lights[i].onColor = currentColor_unit;
					if(preview) {
						obstacle.lights[i].onPeriod_ms = 1000;
						obstacle.lights[i].offPeriod_ms = 0;
					}
					else {
						obstacle.lights[i].onPeriod_ms = 125;
						obstacle.lights[i].offPeriod_ms = 125;
					}
				}
				else if(!preview && (i == nextTopLightIndex || i == nextTopBottomIndex)) {
					obstacle.lights[i].onColor = nextColor_uint;
					obstacle.lights[i].onPeriod_ms = 125;
					obstacle.lights[i].offPeriod_ms = 125;
				}
				else {
					obstacle.lights[i].onColor = 0;
					obstacle.lights[i].onPeriod_ms = ActiveBlock.Light.FOREVER;
					obstacle.lights[i].offPeriod_ms = 0;
				}

				obstacle.lights[i].offColor = 0;
				obstacle.lights[i].transitionOnPeriod_ms = 0;
				obstacle.lights[i].transitionOffPeriod_ms = 0;
			}
		}
	}

	public void TestLights()
	{
		for(int obstacleIndex=0; obstacleIndex < obstacles.Count; obstacleIndex++) {
			ActiveBlock obstacle = obstacles[obstacleIndex];
			obstacle.relativeMode = 0;

			for(int i = 0; i < obstacle.lights.Length; ++i) {
				
				if(i == testLightIndex) {
					obstacle.lights[i].onColor = currentColor_unit;
					obstacle.lights[i].onPeriod_ms = 125;
					obstacle.lights[i].offPeriod_ms = 125;
				}
				else {
					obstacle.lights[i].onColor = 0;
					obstacle.lights[i].onPeriod_ms = 1000;
					obstacle.lights[i].offPeriod_ms = 0;
				}
				
				obstacle.lights[i].offColor = 0;
				obstacle.lights[i].transitionOnPeriod_ms = 0;
				obstacle.lights[i].transitionOffPeriod_ms = 0;
			}
		}
	}

	protected override void Enter_RESULTS() {
		base.Enter_RESULTS();

		if(win) {
			text_resultsTitle.text = "Slalom Completed!";
			CelebrationLights();
			button_PlayAgain.gameObject.SetActive(true);
			button_Rebuild.gameObject.SetActive(false);
		}
		else {
			text_resultsTitle.text = "Game Over";
			DisqualifiedLights();

			button_PlayAgain.gameObject.SetActive(false);
			button_Rebuild.gameObject.SetActive(true);
		}
	}

	private void DisqualifiedLights()
	{
		for(int obstacleIndex = 0; obstacleIndex < obstacles.Count; ++obstacleIndex) {
			ActiveBlock obstacle = obstacles[obstacleIndex];
			obstacle.relativeMode = 0;
			
			for(int i = 0; i < obstacle.lights.Length; ++i) {
				obstacle.SetLEDs(CozmoPalette.ColorToUInt(Color.red), 0, byte.MaxValue, 250, 250);
			}
		}
		
		for(int i = 0; i < robot.lights.Length; ++i) {
			robot.SetBackpackLEDs(CozmoPalette.ColorToUInt(Color.red), 0, byte.MaxValue, 250, 250);
		}
	}

	private void CelebrationLights()
	{
		int random;
		int max = (int)ActiveBlock.Mode.Count-3;

		for(int obstacleIndex = 0; obstacleIndex < obstacles.Count; ++obstacleIndex) {
			ActiveBlock obstacle = obstacles[obstacleIndex];
			obstacle.relativeMode = 0;
			random = Random.Range(1, max);

			for(int i = 0; i < obstacle.lights.Length; ++i) {
				obstacle.lights[i].onColor = CycleColors(i);
				obstacle.lights[i].onPeriod_ms = 125;
				obstacle.lights[i].offPeriod_ms = 125;
				obstacle.lights[i].offColor = CycleColors(i + random);
				obstacle.lights[i].transitionOnPeriod_ms = 0;
				obstacle.lights[i].transitionOffPeriod_ms = 0;
			}
		}

		random = Random.Range(1, max);

		for(int i = 0; i < robot.lights.Length; ++i) {
			robot.lights[i].onColor = CycleColors(i);
			robot.lights[i].onPeriod_ms = 125;
			robot.lights[i].offPeriod_ms = 125;
			robot.lights[i].offColor = CycleColors(i + random);
			robot.lights[i].transitionOnPeriod_ms = 0;
			robot.lights[i].transitionOffPeriod_ms = 0;
		}
	}

	private uint CycleColors(int i) // cycle through all colors, but don't use black or white
	{
		int index = (i % (int)ActiveBlock.Mode.Count) + 2;

		return CozmoPalette.instance.GetUIntColorForActiveBlockType(index < (int)ActiveBlock.Mode.Count ? (ActiveBlock.Mode)index : (ActiveBlock.Mode)2);
	}
}
