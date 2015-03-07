using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class SlalomController : GameController {

	[SerializeField] bool thereAndBackAgain = true;
	[SerializeField] bool endless = true; //if not endless, then its basically a timed slalom trial
	[SerializeField] bool firstPassClockwise = true;
	[SerializeField] bool firstPassEitherDirection = true;

	bool currentPassClockwise = true;

	List<ObservedObject> obstacles = new List<ObservedObject>();
	int currentObstacleIndex = 0;
	bool forward = true;
	Robot robot;
	Vector2 lastRobotPos;
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

		//design query: how to order our obstacles?  by distance from coz at game start?
		//
		foreach(ObservedObject obj in robot.observedObjects) {
			//if(obj.objectType == ObservedObjectType.Gold) {
				obstacles.Add(obj);
			//}
		}

		lastRobotPos = robot.WorldPosition;
		currentObjPos = currentObstacle.WorldPosition;
		nextObjPos = nextObstacle.WorldPosition;
	}

	protected override void Update_PLAYING() {
		base.Update_PLAYING();


		Vector2 cozPos = robot.WorldPosition;

		//design query: calc if we've crossed a quadrant of our current obstacle cube and signal it to light it's corner?
		//use lastPassClockwise to determine if we are orbiting in the right dir
		// not sure how the orbiting behavior works in classic slalom runs (ie, zig zags instead of the figure eights of our default case)

		//only check this if our current obstacle cube has three corners lit already?
		if(MathUtil.AreLineSegmentsCrossing(cozPos, lastRobotPos, currentObjPos, nextObjPos)) {
			//Vector2 delta = cozPos - lastRobotPos;
			Vector2 cozToCurrent = currentObjPos - cozPos;
			Vector2 tape = nextObjPos - currentObjPos;
			if((firstPassEitherDirection && !passedFirstObstacle) || (Vector2.Dot(cozToCurrent, tape.normalized) < 0f ^ currentPassClockwise)) {
				TapeCrossed();
			}
			else {
				//design query: crossed between obstacles in the wrong direction?
				//scores[0]--;
			}
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

	void TapeCrossed() {

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
