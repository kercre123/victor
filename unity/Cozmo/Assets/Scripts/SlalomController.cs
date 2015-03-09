using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class SlalomController : GameController {

	[SerializeField] bool thereAndBackAgain = true;	//when reaching the end or beginning of obstacle list, reverse order
	[SerializeField] bool endless = true; //if not endless, then its basically a timed slalom trial
	[SerializeField] bool firstPassClockwise = true;
	[SerializeField] bool firstPassEitherDirection = true;
	[SerializeField] bool trackWithCornerTriggers = false;

	bool currentPassClockwise = true;

	List<ObservedObject> obstacles = new List<ObservedObject>();
	int currentObstacleIndex = 0;
	bool forward = true;
	Robot robot;
	Vector2 lastRobotPos;
	float lastAngleFromObstacle;
	Vector2 crossDirection;
	Vector2 currentObjPos;
	Vector2 nextObjPos;
	bool courseCompleted = false;
	bool passedFirstObstacle = false;


	ObservedObject currentObstacle { get { return obstacles[currentObstacleIndex]; } }
	ObservedObject nextObstacle { 
		get { 
			int nextIndex;
			if(forward) {
				nextIndex = currentObstacleIndex + 1;
				if(nextIndex >= obstacles.Count) {
					if(thereAndBackAgain) {
						forward = false;
						nextIndex = obstacles.Count-2;
					}
					else {
						nextIndex = 0;
						if(!endless) courseCompleted = true;
					}
				}
			}
			else {
				nextIndex = currentObstacleIndex - 1;
				if(nextIndex < 0) {
					if(thereAndBackAgain) {

						forward = true;
						nextIndex = 1;
						if(!endless) courseCompleted = true;
					}
					else {
						nextIndex = obstacles.Count-1;
					}
				}
			}
			return obstacles[nextIndex];
		}
	}

	protected override void Enter_PLAYING() {
		base.Enter_PLAYING();

		passedFirstObstacle = false;
		currentPassClockwise = firstPassClockwise;
		courseCompleted = false;
		currentObstacleIndex = 0;
		obstacles.Clear();
		robot = RobotEngineManager.instance.current;

		foreach(ObservedObject obj in robot.observedObjects) {
			//if(obj.objectType == ObservedObjectType.Gold) {
				obstacles.Add(obj);
			//}
		}

		//design query: how to order our obstacles?  by distance from coz at game start?
		obstacles.Sort( 
			delegate(ObservedObject obj1, ObservedObject obj2) {
				float d1 = (obj1.WorldPosition - robot.WorldPosition).sqrMagnitude;
				float d2 = (obj2.WorldPosition - robot.WorldPosition).sqrMagnitude;

				return d1.CompareTo(d2);   
			}
		);

		if(trackWithCornerTriggers) {
			//activate corners on first cube
			//pulse next corner
		}

		lastRobotPos = robot.WorldPosition;
		currentObjPos = currentObstacle.WorldPosition;

		//we might as well handle the one obstacle case for robustness
		bool multiObstacle = nextObstacle != null && nextObstacle != currentObstacle;
		if(multiObstacle) nextObjPos = nextObstacle.WorldPosition;

		lastAngleFromObstacle = Vector2.Angle(Vector2.up, lastRobotPos - currentObjPos);
	}

	protected override void Update_PLAYING() {
		base.Update_PLAYING();

		Vector2 cozPos = robot.WorldPosition;

		currentObjPos = currentObstacle.WorldPosition;
		bool multiObstacle = nextObstacle != null && nextObstacle != currentObstacle;
		if(multiObstacle) {
			nextObjPos = nextObstacle.WorldPosition;
		}

		float newAngleFromObstacle = Vector2.Angle(Vector2.up, cozPos - currentObjPos);

		//design query: calc if we've crossed a quadrant of our current obstacle cube and signal it to light it's corner?
		//need object rotation angle to do this properly
		//use lastPassClockwise to determine if we are orbiting in the right dir
		// not sure how the orbiting behavior works in classic slalom runs (ie, zig zags instead of the figure eights of our default case)

		if(trackWithCornerTriggers) {
			//if coz is crossed over active corner angle since last frame, NextCorner();
		}
		else if(multiObstacle) {
			//only check this if our current obstacle cube has three corners lit already?
			if(MathUtil.AreLineSegmentsCrossing(cozPos, lastRobotPos, currentObjPos, nextObjPos)) {
				//Vector2 delta = cozPos - lastRobotPos;
				Vector2 cozToCurrent = currentObjPos - cozPos;
				Vector2 tape = nextObjPos - currentObjPos;
				if((firstPassEitherDirection && !passedFirstObstacle) || (Vector2.Dot(cozToCurrent, tape.normalized) < 0f ^ currentPassClockwise)) {
					NextObstacle();
				}
				else {
					//design query: crossed between obstacles in the wrong direction?
					//scores[0]--;
				}
			}
		}
		else {
			//todo: single obstacle mode just records laps
		}

		lastRobotPos = cozPos;
	}

	protected override bool IsGameReady() {
		if(!base.IsGameReady()) return false;
		
		//game specific start conditions...

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

		passedFirstObstacle = true;

		scores[0]++;

		//design query: reset lights on current cube now or when we've completed a circuit?
		if(forward) {
			currentObstacleIndex++;
			if(currentObstacleIndex >= obstacles.Count) {
				if(thereAndBackAgain) {
					forward = false;
					currentObstacleIndex = obstacles.Count-2;
				}
				else if(endless) {
					currentObstacleIndex = 0;
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
				else {
					currentObstacleIndex = obstacles.Count-1;
				}
			}
		}

		currentPassClockwise = !currentPassClockwise;
	}

}
