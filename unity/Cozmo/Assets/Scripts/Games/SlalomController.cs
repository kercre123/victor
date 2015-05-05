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
	//Vector2 lastRobotPos;
	float lastAngleFromObstacle;
	float totalAngleFromObstacleTraversed;
	bool courseCompleted = false;
	int tapesCrossed = 0;

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

			int nextIndex = 0;
			if(forward) {
				nextIndex = currentObstacleIndex + 1;
				if(nextIndex >= obstacles.Count) {
					if(thereAndBackAgain) {
						nextIndex = obstacles.Count-2;
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
						nextIndex = obstacles.Count-1;
					}
				}
			}

			if(nextIndex < 0) return null;
			if(nextIndex >= obstacles.Count) return null;

			return obstacles[nextIndex];
		}
	}

	Vector2 currentObstaclePos { 
		get {
			if(obstaclePositions == null) return Vector2.zero;
			if(currentObstacleIndex < 0) return Vector2.zero;
			if(currentObstacleIndex >= obstaclePositions.Count) return Vector2.zero;
			
			return obstaclePositions[currentObstacleIndex];
		}
	}
	Vector2 nextObstaclePos { 
		get { 
			if(obstaclePositions == null) return Vector2.zero;
			if(obstaclePositions.Count == 0) return Vector2.zero;
			if(obstaclePositions.Count == 1) return currentObstaclePos;
			
			int nextIndex = 0;
			if(forward) {
				nextIndex = currentObstacleIndex + 1;
				if(nextIndex >= obstaclePositions.Count) {
					if(thereAndBackAgain) {
						nextIndex = obstaclePositions.Count-2;
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
						nextIndex = obstaclePositions.Count-1;
					}
				}
			}
			
			if(nextIndex < 0) return Vector2.zero;
			if(nextIndex >= obstaclePositions.Count) return Vector2.zero;
			
			return obstaclePositions[nextIndex];
		}
	}

	protected override void OnEnable()
	{
		base.OnEnable();
		
		instance = this;
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

		Vector3 pos = GameLayoutTracker.instance.GetStartingPositionFromLayout();
		float rad = GameLayoutTracker.instance.GetStartingAngleFromLayout();

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
		for(int i=0;i<robot.knownObjects.Count;i++) {
			if(robot.knownObjects[i].Family == 3) {
				//Debug.Log(gameObject.name + " IsGameReady adding a gold block to obstacles." );
				obstacles.Add(robot.knownObjects[i]);
			}
		}
		
		if(obstacles.Count < 2) {
			Debug.LogError(gameObject.name + " Enter_PLAYING found only "+obstacles.Count+" obstacles.  At least two obstacles required to play Slalom." );
		}

		tapesCrossed = 0;
		courseCompleted = false;
		currentObstacleIndex = 0;

		//design query: how to order our obstacles?  by distance from coz at game start?
		obstacles.Sort( 
			delegate(ObservedObject obj1, ObservedObject obj2) {
				float d1 = (obj1.WorldPosition - robot.WorldPosition).sqrMagnitude;
				float d2 = (obj2.WorldPosition - robot.WorldPosition).sqrMagnitude;

				return d1.CompareTo(d2);   
			}
		);

		obstaclePositions.Clear();
		foreach(ObservedObject obj in obstacles) {
			obstaclePositions.Add(obj.WorldPosition);
		}

		if(trackWithCornerTriggers) {
			//activate corners on first cube
			//pulse next corner
		}

		totalAngleFromObstacleTraversed = 0f;
		Vector2 currentObstacleToRobot = (Vector2)robot.WorldPosition - currentObstaclePos;
		lastAngleFromObstacle = Vector2.Angle(Vector2.up, currentObstacleToRobot) * (currentObstacleToRobot.x >= 0 ? 1f : -1f);
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

	public void UpdateLights()
	{
		if( CozmoPalette.instance != null && currentObstacle != null && currentObstacle.Family == 3 && 
		   nextObstacle != null && nextObstacle.Family == 3 )
		{
			uint yellow = CozmoPalette.instance.GetUIntColorForActiveBlockType( ActiveBlockType.Yellow );
			uint red = CozmoPalette.instance.GetUIntColorForActiveBlockType( ActiveBlockType.Red );

			float distanceFromRobot = Vector2.Distance( robot.WorldPosition, nextObstacle.WorldPosition );
			float distanceFromCurrentObstacle = Vector2.Distance( currentObstacle.WorldPosition, nextObstacle.WorldPosition );

			currentObstacle.relativeMode = 1;
			currentObstacle.relativeToX = robot.WorldPosition.x;
			currentObstacle.relativeToY = robot.WorldPosition.y;
			
			for( int i = 0; i < 8; ++i )
			{
				currentObstacle.lights[i].onColor = 0;
				currentObstacle.lights[i].offColor = 0;
				currentObstacle.lights[i].onPeriod_ms = 1000;
				currentObstacle.lights[i].offPeriod_ms = 0;
				currentObstacle.lights[i].transitionOnPeriod_ms = 0;
				currentObstacle.lights[i].transitionOffPeriod_ms = 0;
				
				if( i == 0 || i == 2 )
				{
					currentObstacle.lights[i].onColor = yellow;
				}
				else if( i == 1 || i == 3 )
				{
					if( distanceFromRobot > distanceFromCurrentObstacle )
					{
						currentObstacle.lights[i].onColor = red;
						currentObstacle.lights[i].onPeriod_ms = 125;
						currentObstacle.lights[i].offPeriod_ms = 125;
					}
					else
					{
						nextObstacle.lights[i].onColor = red;
						nextObstacle.lights[i].onPeriod_ms = 125;
						nextObstacle.lights[i].offPeriod_ms = 125;
					}
				}
			}
		}
	}
}
