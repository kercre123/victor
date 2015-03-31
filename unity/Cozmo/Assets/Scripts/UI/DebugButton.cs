using UnityEngine;
using System.Collections;

public class DebugButton : MonoBehaviour
{
	public void ForceDropBox()
	{
		if( RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			RobotEngineManager.instance.current.SetRobotCarryingObject();
		}
	}
	
	public void ForceClearAll()
	{
		if( RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			RobotEngineManager.instance.current.ClearAllBlocks();
		}
	}
}
