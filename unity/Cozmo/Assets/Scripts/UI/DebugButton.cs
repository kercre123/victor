using UnityEngine;
using System.Collections;

public class DebugButton : MonoBehaviour
{
	private Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	public void ForceDropBox()
	{
		if( robot != null )
		{
			robot.SetRobotCarryingObject();
		}
	}
	
	public void ForceClearAll()
	{
		if( robot != null )
		{
			robot.ClearAllBlocks();
		}
	}
}
