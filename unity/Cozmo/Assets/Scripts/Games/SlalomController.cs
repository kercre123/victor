using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class SlalomController : GameController {
	public static SlalomController instance = null;

	[SerializeField] Text text_finalTime;
	[SerializeField] bool thereAndBackAgain = true;	//when reaching the end or beginning of obstacle list, reverse order
	[SerializeField] int pointsPerLap = 100;
	[SerializeField] Text textObservedCount = null;
	[SerializeField] Text textProgress = null;

	[SerializeField] AudioClip cornerTriggeredSound = null;

	List<ActiveBlock> obstacles = new List<ActiveBlock>();

	bool atYourMark = false;

	int currentCorner = 0;
	int currentObstacleIndex = 0;
	bool clockwise = false;
	bool forward = true;

	uint nextColor_uint;
	uint currentColor_unit;

	int currentLap = 1;

	float[] idealCornerAngles = { 45f, 135f, 225f, 315f };

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
		scores[0] += pointsPerLap;
		//Debug.Log("LapComplete scores[0](" + scores[0].ToString() + ")");
		audio.PlayOneShot(playerScoreSound);
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
			nextColor_uint = CozmoPalette.instance.GetUIntColorForActiveBlockType(ActiveBlock.Type.White);
			currentColor_unit = CozmoPalette.instance.GetUIntColorForActiveBlockType(ActiveBlock.Type.Cyan);
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

		float rad;
		Vector3 facing;
		Vector3 pos = GameLayoutTracker.instance.GetStartingPositionFromLayout(out rad, out facing);
		CozmoBusyPanel.instance.SetDescription("Cozmo is getting in the starting position.");
		robot.GotoPose(pos.x, pos.y, rad);
		PrepareGameForPlay(pos, facing);
		atYourMark = false;
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure += CheckForGotoPoseCompletion;
		robot.isBusy = true;
	}

	void CheckForGotoPoseCompletion(bool success, RobotActionType action_type) {

		switch(action_type) {
			case RobotActionType.DRIVE_TO_POSE:
				if(success) {
					atYourMark = true;
					if(RobotEngineManager.instance != null) RobotEngineManager.instance.SuccessOrFailure -= CheckForGotoPoseCompletion;
				}
				else { //try again or fail outright?
					float rad;
					Vector3 facing;
					Vector3 pos = GameLayoutTracker.instance.GetStartingPositionFromLayout(out rad, out facing);
					CozmoBusyPanel.instance.SetDescription("Cozmo is getting in the starting position.");
					robot.GotoPose(pos.x, pos.y, rad);
				}
				break;
		}
	}

	protected override void Update_PRE_GAME() {
		//only let our countdown start when our goto command is done
		//todo make it have to succeed?
		if(!atYourMark) return;

		base.Update_PRE_GAME();
	}

	protected override void Exit_PRE_GAME() {
		robot.isBusy = false;
		base.Exit_PRE_GAME();
	}

	void PrepareGameForPlay(Vector3 startingPos, Vector3 startingFacing) {

		obstacles.Clear();
		List<ObservedObject> observedObjects = GameLayoutTracker.instance.GetTrackedObjectsInOrder().FindAll( x => x.isActive );
		foreach(ObservedObject obj in observedObjects) obstacles.Add(obj as ActiveBlock);

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

		currentCorner = 0;
		bool onNextObstacle;
		int nextCorner = GetNextCorner(out onNextObstacle);
		UpdateCornerLights(currentCorner, nextCorner, onNextObstacle);
	}

	protected override void Enter_PLAYING() {
		base.Enter_PLAYING();
	}

	protected override void Update_PLAYING() {
		base.Update_PLAYING();

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

	protected override void Exit_PLAYING() {
		base.Exit_PLAYING();

		text_finalTime.text = "Final Time: " + stateTimer.ToString("f2") + " seconds";
		ClearLights();
	}

	protected override bool IsGameReady() {
		if(!base.IsGameReady()) return false;
		return true;
	}

	protected override bool IsGameOver() {
		if(base.IsGameOver()) return true;

		//game specific end conditions...

		return false;
	}

	Vector2 GetCornerVector(ActiveBlock obstacle, int cornerIndex, bool clock, ActiveBlock previous, bool debug=false) {

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

	int lastLapCount = 1;

	void AdvanceCorner() {

		//trigger lap score and sound when we cross our first corner again
		if(currentCorner == 0 && lastLapCount != currentLap) {
			LapComplete();
		}

		lastLapCount = currentLap;

		bool obstacleAdvanced;
		currentCorner = GetNextCorner(out obstacleAdvanced);

		if(obstacleAdvanced) {
			previousObstacle = currentObstacle;
			currentObstacleIndex = GetNextIndex(true);
			clockwise = !clockwise;
		}

		bool nextCornerIsOnNextObstacle;
		int nextCorner = GetNextCorner(out nextCornerIsOnNextObstacle);

		audio.PlayOneShot(cornerTriggeredSound);

		//Debug.Log("AdvanceCorner obstacleAdvanced("+obstacleAdvanced+") clockwise("+clockwise+") currentCorner("+currentCorner+") nextCorner("+nextCorner+") nextCornerIsOnNextObstacle("+nextCornerIsOnNextObstacle+")");

		UpdateCornerLights(currentCorner, nextCorner, nextCornerIsOnNextObstacle);
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

	public void UpdateCornerLights(int corner, int next, bool nextCornerOnNextObstacle, bool debug=false)
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
			for(int i = 0; i < 8; ++i) {

				if(i == currentTopLightIndex || i == currentBottomLightIndex) {
					obstacle.lights[i].onColor = currentColor_unit;
					obstacle.lights[i].onPeriod_ms = 125;
					obstacle.lights[i].offPeriod_ms = 125;
				}
				else if(i == nextTopLightIndex || i == nextTopBottomIndex) {
					obstacle.lights[i].onColor = nextColor_uint;
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

	public void ClearLights()
	{

		for(int obstacleIndex=0; obstacleIndex < obstacles.Count; obstacleIndex++) {
			ActiveBlock obstacle = obstacles[obstacleIndex];
			obstacle.relativeMode = 0;
			for(int i = 0; i < 8; ++i) {
				obstacle.lights[i].onColor = 0;
				obstacle.lights[i].onPeriod_ms = 1000;
				obstacle.lights[i].offPeriod_ms = 0;
				obstacle.lights[i].offColor = 0;
				obstacle.lights[i].transitionOnPeriod_ms = 0;
				obstacle.lights[i].transitionOffPeriod_ms = 0;
			}
		}
	}


	public int testLightIndex = 0;
	public void TestLights()
	{
		for(int obstacleIndex=0; obstacleIndex < obstacles.Count; obstacleIndex++) {
			ActiveBlock obstacle = obstacles[obstacleIndex];
			obstacle.relativeMode = 0;

			for(int i = 0; i < 8; ++i) {
				
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
}
