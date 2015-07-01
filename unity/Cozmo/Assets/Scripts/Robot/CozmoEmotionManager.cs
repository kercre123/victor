using UnityEngine;
using System.Collections;
using U2G = Anki.Cozmo.U2G;

public class CozmoEmotionManager : MonoBehaviour {

	public static CozmoEmotionManager instance = null;
	private U2G.PlayAnimation PlayAnimationMessage;

	void Awake()
	{
		PlayAnimationMessage = new U2G.PlayAnimation();
		instance = this;
	}

	// Use this for initialization
	void Start () {
	
	}
	
	// Update is called once per frame
	void Update () {
		if( Input.GetKeyDown(KeyCode.L) )
		{
			SendAnimation("ANIM_TEST", 1);
		}
	
	}

	public void SendAnimation(string anim_name, uint num_loops)
	{
		PlayAnimationMessage.animationName = anim_name;
		PlayAnimationMessage.numLoops = num_loops;
		RobotEngineManager.instance.Message.PlayAnimation = PlayAnimationMessage;
		RobotEngineManager.instance.SendMessage();
	}

}
