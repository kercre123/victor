using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class SlalomController : GameController {

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
	Robot robot;
	//Vector2 lastRobotPos;
	float lastAngleFromObstacle;
	float totalAngleFromObstacleTraversed;
	bool courseCompleted = false;
	int tapesCrossed = 0;

	private float timeDelay = 0f;

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

	protected override void Update_BUILDING() {
		base.Update_BUILDING();

		if(textObservedCount != null) {
			textObservedCount.text = "obstacles: " +obstacles.Count.ToString();
		}

	}

	protected override void Enter_PLAYING() {
		base.Enter_PLAYING();

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
	}

	protected override bool IsGameReady() {
		if(!base.IsGameReady()) return false;
		
		//game specific start conditions...
		robot = RobotEngineManager.instance.current;
		
		obstacles.Clear();
		for(int i=0;i<robot.knownObjects.Count;i++) {
			//if(robot.knownObjects[i].objectType == ObservedObjectType.Gold) {
			//Debug.Log(gameObject.name + " IsGameReady adding a gold block to obstacles." );
			obstacles.Add(robot.knownObjects[i]);
			//}
		}

		return obstacles.Count > 1;
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
		if( Time.time > timeDelay && CozmoPalette.instance != null && 
		   currentObstacle != null && currentObstacle.Family == 3 && nextObstacle != null && nextObstacle.Family == 3 &&
		   robot.LastWorldPosition != robot.WorldPosition && robot.LastRotation != robot.Rotation )
		{
			uint yellow = CozmoPalette.instance.GetUIntColorForActiveBlockType( ActiveBlockType.Yellow );
			uint red = CozmoPalette.instance.GetUIntColorForActiveBlockType( ActiveBlockType.Red );

			float distanceFromRobot = Vector2.Distance( robot.WorldPosition, nextObstacle.WorldPosition );
			float distanceFromCurrentObstacle = Vector2.Distance( currentObstacle.WorldPosition, nextObstacle.WorldPosition );
				
			for( int i = 0; i < 8; ++i )
			{
				currentObstacle.onColor[i] = 0;
				currentObstacle.offColor[i] = 0;
				currentObstacle.onPeriod_ms[i] = 1000;
				currentObstacle.offPeriod_ms[i] = 0;
				currentObstacle.transitionOnPeriod_ms[i] = 0;
				currentObstacle.transitionOffPeriod_ms[i] = 0;
				
				if( i == 0 || i == 2 )
				{
					currentObstacle.onColor[i] = yellow;
				}
				else if( i == 1 || i == 3 )
				{
					if( distanceFromRobot > distanceFromCurrentObstacle )
					{
						currentObstacle.onColor[i] = red;
						currentObstacle.onPeriod_ms[i] = 125;
						currentObstacle.offPeriod_ms[i] = 125;
					}
					else
					{
						nextObstacle.onColor[i] = red;
						nextObstacle.onPeriod_ms[i] = 125;
						nextObstacle.offPeriod_ms[i] = 125;
					}
				}
			}

			currentObstacle.SetAllActiveObjectLEDs( 1, robot.WorldPosition.x, robot.WorldPosition.y );
			
			timeDelay = Time.time + 0.5f;
		}
	}
}
