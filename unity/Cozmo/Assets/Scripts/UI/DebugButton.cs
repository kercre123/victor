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

	public void ForceClearStars()
	{
		for(int i = 0; i < GameData.instance.games.Length; i++)
		{
			GameData.Game game = GameData.instance.games[i];
			for(int j = 0; j < game.levels.Length; j++)
			{
				string stars_string = game.name+game.levels[j].name+GameController.STARS_END;
				Debug.Log(game.name+game.levels[j].name+", current stars: "+ PlayerPrefs.GetInt(stars_string,0));
				PlayerPrefs.SetInt(stars_string, 0);
			}
		}
	}
}
