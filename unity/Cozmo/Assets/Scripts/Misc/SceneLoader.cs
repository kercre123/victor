using UnityEngine;
using System.Collections;

/// <summary>
/// allows ui interactions, such as button presses, to trigger a LevelLoad
/// </summary>
public class SceneLoader : MonoBehaviour
{

	[SerializeField] string scene = null;
	[SerializeField] bool onEnable = false;
	[SerializeField] int onUpdateFrame = -1;
	[SerializeField] bool disconnectFromRobot = false;

	int frames = 0;

	void OnEnable()
	{
		if(onEnable) LoadScene();
	}

	void Update()
	{
		frames++;
		if(onUpdateFrame == frames) LoadScene();
	}

	public void LoadScene()
	{
		if(string.IsNullOrEmpty(scene)) return;

		if (disconnectFromRobot) {
			RobotEngineManager.instance.Disconnect();
		}

		Debug.Log("SceneLoader Application.LoadLevel(" + scene + ")");
		Application.LoadLevel(scene);
	}
}
