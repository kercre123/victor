using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class SlalomController : GameController {
	public static SlalomController instance = null;

	[SerializeField] Text text_finalTime;

	[SerializeField] bool thereAndBackAgain = true;	//when reaching the end or beginning of obstacle list, reverse order
	[SerializeField] bool endless = true; //if not endless, then its basically a timed slalom trial
	[SerializeField] bool trackWithCornerTriggers = false;
	[SerializeField] Text textObservedCount = null;
	[SerializeField] Text textProgress = null;

	List<ObservedObject> obstacles = new List<ObservedObject>();
	List<Vector2> obstaclePositions = new List<Vector2>();

	int currentObstacleIndex = 0;
	bool lastPassCrossUp = true;
	bool forward = true;
	float lastAngleFromObstacle;
	float totalAngleFromObstacleTraversed;
	bool courseCompleted = false;
	int tapesCrossed = 0;

	ObservedObject previousObstacle = null;

	ObservedObject currentObstacle { 
		get {
			if(obstacles == null) return null;
			if(currentObstacleIndex < 0) return null;
			if(currentObstacleIndex >= obstacles.Count) return null;

			return obstacles[currentObstacleIndex];
		}
	}
	ObservedObject nextObstacle { 
		get { 
			if(obstacles == null) return null;

			int nextIndex = GetNextIndex();

			if(nextIndex < 0) return null;
			if(nextIndex >= obstacles.Count) return null;

			return obstacles[nextIndex];
		}
	}

	Vector2 currentObstaclePos { 
		get {
			return currentObstacle.WorldPosition;
		}
	}

	Vector2 nextObstaclePos { 
		get { 
			return nextObstacle.WorldPosition;
		}
	}

	int GetNextIndex() {
		int nextIndex = 0;
		if(forward) {
			nextIndex = currentObstacleIndex + 1;
			if(nextIndex >= obstaclePositions.Count) {
				if(thereAndBackAgain) {
					nextIndex = obstaclePositions.Count - 2;
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
					nextIndex = obstaclePositions.Count - 1;
				}
			}
		}
	
		return nextIndex;
	}

	int GetPreviousIndex() {
		int nextIndex = 0;
		if(!forward) {
			nextIndex = currentObstacleIndex + 1;
			if(nextIndex >= obstaclePositions.Count) {
				if(thereAndBackAgain) {
					nextIndex = obstaclePositions.Count - 2;
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
					nextIndex = obstaclePositions.Count - 1;
				}
			}
		}
		
		return nextIndex;
	}

	uint nextColor_uint;
	uint currentColor_unit;
	protected override void OnEnable()
	{
		base.OnEnable();
		
		instance = this;

		if(CozmoPalette.instance != null) {
			nextColor_uint = CozmoPalette.instance.GetUIntColorForActiveBlockType(ActiveBlockType.White);
			currentColor_unit = CozmoPalette.instance.GetUIntColorForActiveBlockType(ActiveBlockType.Cyan);
		}
	}
	
	protected override void OnDisable()
	{
		base.OnEnable();
		
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

		robot = RobotEngineManager.instance.current;
		float rad;
		Vector3 pos = GameLayoutTracker.instance.GetStartingPositionFromLayout(out rad);
		CozmoBusyPanel.instance.SetDescription("Cozmo is getting in the starting position.");
		robot.GotoPose(pos.x, pos.y, rad);

	}

	protected override void Update_PRE_GAME() {

		robot = RobotEngineManager.instance.current;

		//only let our countdown start when our goto command is done
		//todo make it have to succeed?
		if(robot.isBusy) return;

		base.Update_PRE_GAME();
	}

	protected override void Enter_PLAYING() {
		base.Enter_PLAYING();

		//game specific start conditions...
		robot = RobotEngineManager.instance.current;
		
		obstacles.Clear();
		obstacles.AddRange(GameLayoutTracker.instance.GetTrackedObjectsInOrder());

		//cull the inactive blocks
		for(int i=0;i<obstacles.Count;i++) {
			if(obstacles[i].Family != 3) {
				//Debug.Log(gameObject.name + " IsGameReady adding a gold block to obstacles." );
				obstacles.Remove(obstacles[i]);
			}
		}
		
		if(obstacles.Count < 2) {
			Debug.LogError(gameObject.name + " Enter_PLAYING found only "+obstacles.Count+" obstacles.  At least two obstacles required to play Slalom." );
		}

		tapesCrossed = 0;
		courseCompleted = false;
		currentObstacleIndex = 0;

		//design query: how to order our obstacles?  by distance from coz at game start?
//		obstacles.Sort( 
//			delegate(ObservedObject obj1, ObservedObject obj2) {
//				float d1 = (obj1.WorldPosition - robot.WorldPosition).sqrMagnitude;
//				float d2 = (obj2.WorldPosition - robot.WorldPosition).sqrMagnitude;
//
//				return d1.CompareTo(d2);   
//			}
//		);

		if(trackWithCornerTriggers) {
			//activate corners on first cube
			//pulse next corner
		}

		totalAngleFromObstacleTraversed = 0f;
		Vector2 currentObstacleToRobot = (Vector2)robot.WorldPosition - currentObstaclePos;
		lastAngleFromObstacle = Vector2.Angle(Vector2.up, currentObstacleToRobot) * (currentObstacleToRobot.x >= 0 ? 1f : -1f);

		if(thereAndBackAgain) {
			previousObstacle = nextObstacle;
		}
		else {
			int index = currentObstacleIndex-1;
			if(index < 0) index = obstacles.Count-1;
			previousObstacle = obstacles[index];
		}
	}

	protected override void Update_PLAYING() {
		base.Update_PLAYING();

		Vector2 robotPos = robot.WorldPosition;
		Vector2 lastRobotPos = robot.LastWorldPosition;

		Vector2 currentObstacleToRobot = robotPos - currentObstaclePos;
		float newAngleFromObstacle = Vector2.Angle(Vector2.up, currentObstacleToRobot.normalized) * (currentObstacleToRobot.x >= 0 ? 1f : -1f);
		float angleDeltaThisFrame = MathUtil.AngleDelta(lastAngleFromObstacle, newAngleFromObstacle);

		totalAngleFromObstacleTraversed += angleDeltaThisFrame;
		//Debug.Log("oldAngle("+lastAngleFromObstacle+")->newAngle("+newAngleFromObstacle+") delta("+angleDeltaThisFrame+") totalAngleFromObstacleTraversed("+totalAngleFromObstacleTraversed+")" );

		lastAngleFromObstacle = newAngleFromObstacle;
		//design query: calc if we've crossed a quadrant of our current obstacle cube and signal it to light it's corner?
		//need object rotation angle to do this properly
		//use lastPassClockwise to determine if we are orbiting in the right dir
		// not sure how the orbiting behavior works in classic slalom runs (ie, zig zags instead of the figure eights of our default case)

		UpdateLights();

		if(trackWithCornerTriggers) {
			//if coz is crossed over active corner angle since last frame, NextCorner();
		}
		else if(obstaclePositions.Count > 1) {
			//only check this if our current obstacle cube has three corners lit already?
			bool allowedToCrossTape = tapesCrossed == 0 || Mathf.Abs(totalAngleFromObstacleTraversed) > 90f;

			if(allowedToCrossTape && MathUtil.AreLineSegmentsCrossing(robotPos, lastRobotPos, currentObstaclePos, nextObstaclePos)) {
				Vector2 delta = robotPos - lastRobotPos;
				Vector2 tape = nextObstaclePos - currentObstaclePos;
				Vector3 cross = Vector3.Cross(delta.normalized, tape.normalized);

				if(tapesCrossed == 0 || (cross.z > 0f ^ lastPassCrossUp)) {
					lastPassCrossUp = cross.z > 0f;
					NextObstacle();
				}
				else {
					//design query: crossed between obstacles in the wrong direction?
					//tapesCrossed = scores[0] - 1;
					//Debug.Log("Update_PLAYING currentObstacleIndex("+currentObstacleIndex+") crossed between obstacles in the wrong direction currentPassClockwise("+lastPassCrossUp+")");
				}
			}
		}
		else {
			//todo: single obstacle mode just records laps
			Debug.Log("Update_PLAYING currentObstacleIndex("+currentObstacleIndex+") single obstacle? currentPassClockwise("+lastPassCrossUp+")");
		}

		if(textObservedCount != null) {
			textObservedCount.text = "obstacles: " +obstacles.Count.ToString();
		}

		if(textProgress != null) {
			textProgress.text = "index("+currentObstacleIndex+") angle("+totalAngleFromObstacleTraversed+")";
		}

		if(maxPlayTime > 0f) {
			int remaining = Mathf.CeilToInt(Mathf.Clamp(maxPlayTime - stateTimer, 0, maxPlayTime));

			if(remaining <= 30f) {

				PlayCountdownAudio(remaining);
				
				if(countdownText != null) {
					
					countdownText.text = remaining.ToString();
					countdownText.gameObject.SetActive(true);
				}
			}
			else
			if(countdownText != null) {
				countdownText.gameObject.SetActive(false);
			}
		}

	}

	protected override void Exit_PLAYING() {
		base.Exit_PLAYING();

		text_finalTime.text = "Final Time: " + stateTimer.ToString("f2") + " seconds";
	}

	protected override bool IsGameReady() {
		if(!base.IsGameReady()) return false;
		return true;
	}

	protected override bool IsGameOver() {
		if(base.IsGameOver()) return true;

		//game specific end conditions...

		if(courseCompleted) return true;

		return false;
	}

	void NextCorner() {

		//activate corner
		//if this is last active corner of cube, NextObstacle()
		//pulse next target corner

	}

	void NextObstacle() {

		tapesCrossed++;

		audio.PlayOneShot(playerScoreSound);

		scores[0] = tapesCrossed;

		previousObstacle = currentObstacle;

		//design query: reset lights on current cube now or when we've completed a circuit?
		if(forward) {
			currentObstacleIndex++;
			if(currentObstacleIndex >= obstaclePositions.Count) {

				if(thereAndBackAgain) {
					forward = false;
					currentObstacleIndex = obstaclePositions.Count-2;
				}
				else {//looping course
					currentObstacleIndex = 0;
					if(!endless) courseCompleted = true;
				}
			}
		}
		else {
			currentObstacleIndex--;
			if(currentObstacleIndex < 0) {
				if(thereAndBackAgain) {
					forward = true;
					currentObstacleIndex = 1;
				}
				else {//looping course
					currentObstacleIndex = obstaclePositions.Count-1;
				}

				//nonEndless thereAndBackAgain ends when it gets back passed the first obstacle again
				if(!endless) courseCompleted = true;
			}
		}

		totalAngleFromObstacleTraversed = 0f;

		Vector2 robotPos = robot.WorldPosition;
		
		Vector2 currentObstacleToRobot = robotPos - currentObstaclePos;
		lastAngleFromObstacle = Vector2.Angle(Vector2.up, currentObstacleToRobot.normalized) * (currentObstacleToRobot.x >= 0 ? 1f : -1f);

		Debug.Log("NextObstacle scores("+scores[0]+") currentObstacleIndex("+currentObstacleIndex+") lastPassCrossUp("+lastPassCrossUp+")");
	}

	class AngularLightCommandInfo {
		public ObservedObject block = null;
		public float angle = 0f;
		public Vector2 lastTape = Vector2.up;
	}

	float[] anglesToShowLightsFrom = { 45f, 135f, 225f, 315f };

	AngularLightCommandInfo currentLightInfo = new AngularLightCommandInfo();
	AngularLightCommandInfo nextLightInfo = new AngularLightCommandInfo();

	public void UpdateLights()
	{
		if(robot == null) return;
		if(currentObstacle == null) return;
		if(currentObstacle.Family != 3) return;
		if(nextObstacle == null) return;
		if(nextObstacle.Family != 3) return;

		Vector2 currentToLast = previousObstacle.WorldPosition - currentObstacle.WorldPosition;
		Vector2 currentToNext = nextObstacle.WorldPosition - currentObstacle.WorldPosition;

		float totalAngleToTraverseOnThisLeg = Vector3.Angle(currentToLast, currentToNext);

		float finalLightAngleForCurrentObstacle = 315f;
		for(int i=anglesToShowLightsFrom.Length-1; i>=0; i--) {
			if(totalAngleToTraverseOnThisLeg < anglesToShowLightsFrom[i]) continue;

			finalLightAngleForCurrentObstacle = anglesToShowLightsFrom[i];
			break;
		}

		if(totalAngleFromObstacleTraversed > finalLightAngleForCurrentObstacle) {
			currentLightInfo.block = nextObstacle;
			currentLightInfo.angle = anglesToShowLightsFrom[0];
			currentLightInfo.lastTape = currentToLast;
			nextLightInfo.block = nextObstacle;
			nextLightInfo.angle = anglesToShowLightsFrom[1];
			nextLightInfo.lastTape = currentToNext;
		}
		else if(totalAngleFromObstacleTraversed > anglesToShowLightsFrom[2]) {
			currentLightInfo.block = currentObstacle;
			currentLightInfo.angle = anglesToShowLightsFrom[3];
			currentLightInfo.lastTape = currentToLast;
			nextLightInfo.block = nextObstacle;
			nextLightInfo.angle = anglesToShowLightsFrom[0];
			nextLightInfo.lastTape = currentToNext;
		}
		else if(totalAngleFromObstacleTraversed > anglesToShowLightsFrom[1]) {
			currentLightInfo.block = currentObstacle;
			currentLightInfo.angle = anglesToShowLightsFrom[2];

			if(finalLightAngleForCurrentObstacle >= anglesToShowLightsFrom[3]) {
				nextLightInfo.block = currentObstacle;
				nextLightInfo.angle = anglesToShowLightsFrom[3];
				nextLightInfo.lastTape = currentToLast;
			}
			else {
				nextLightInfo.block = nextObstacle;
				nextLightInfo.angle = anglesToShowLightsFrom[0];
				nextLightInfo.lastTape = currentToNext;
			}
		}
		else if(totalAngleFromObstacleTraversed > anglesToShowLightsFrom[0]) {
			currentLightInfo.block = currentObstacle;
			currentLightInfo.angle = anglesToShowLightsFrom[1];
			
			if(finalLightAngleForCurrentObstacle >= anglesToShowLightsFrom[2]) {
				nextLightInfo.block = currentObstacle;
				nextLightInfo.angle = anglesToShowLightsFrom[2];
				nextLightInfo.lastTape = currentToLast;
			}
			else {
				nextLightInfo.block = nextObstacle;
				nextLightInfo.angle = anglesToShowLightsFrom[0];
				nextLightInfo.lastTape = currentToNext;
			}
		}
		else {
			currentLightInfo.block = currentObstacle;
			currentLightInfo.angle = anglesToShowLightsFrom[0];
			
			if(finalLightAngleForCurrentObstacle >= anglesToShowLightsFrom[1]) {
				nextLightInfo.block = currentObstacle;
				nextLightInfo.angle = anglesToShowLightsFrom[1];
				nextLightInfo.lastTape = currentToLast;
			}
			else {
				nextLightInfo.block = nextObstacle;
				nextLightInfo.angle = anglesToShowLightsFrom[0];
				nextLightInfo.lastTape = currentToNext;
			}
		}

		for(int obstacleIndex=0; obstacleIndex < obstacles.Count; obstacleIndex++) {
			ObservedObject obstacle = obstacles[obstacleIndex];

			int currentTopLightIndex = -1;
			int currentBottomLightIndex = -1;
			int nextTopLightIndex = -1;
			int nextTopBottomIndex = -1;

			Vector2 obstacleNorth = obstacle.TopNorth;

			if(obstacle == currentLightInfo.block) {
				bool clockwise = lastPassCrossUp ^ obstacle == currentObstacle;
				Vector3 lightVector = Quaternion.AngleAxis(clockwise ? -currentLightInfo.angle : currentLightInfo.angle, Vector3.forward) * currentLightInfo.lastTape.normalized;
				float angleFromNorth = MathUtil.SignedVectorAngle(obstacleNorth, lightVector, Vector3.forward);

				currentTopLightIndex = Light.GetIndexForCornerClosestToAngle(angleFromNorth, true);
				currentBottomLightIndex = Light.GetIndexForCornerClosestToAngle(angleFromNorth, false);

				Debug.Log("currentLightInfo.block("+obstacleIndex+") currentLightInfo.angle("+currentLightInfo.angle+") angleFromNorth("+angleFromNorth+")");
			}

			if(obstacle == nextLightInfo.block) {
				bool clockwise = lastPassCrossUp ^ obstacle == currentObstacle;
				Vector3 lightVector = Quaternion.AngleAxis(clockwise ? -nextLightInfo.angle : nextLightInfo.angle, Vector3.forward) * nextLightInfo.lastTape.normalized;
				float angleFromNorth = MathUtil.SignedVectorAngle(obstacleNorth, lightVector, Vector3.forward);
				
				nextTopLightIndex = Light.GetIndexForCornerClosestToAngle(angleFromNorth, true);
				nextTopBottomIndex = Light.GetIndexForCornerClosestToAngle(angleFromNorth, false);

				Debug.Log("nextLightInfo.block("+obstacleIndex+") nextLightInfo.angle("+nextLightInfo.angle+") angleFromNorth("+angleFromNorth+")");
			}

			//refresh light settings
			for(int i = 0; i < 8; ++i) {

				obstacle.relativeMode = 0;

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
}
