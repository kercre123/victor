using UnityEngine;
using System.Collections;
using U2G = Anki.Cozmo.U2G;

public class CozmoEmotionManager : MonoBehaviour {

	public static CozmoEmotionManager instance = null;
	private U2G.PlayAnimation PlayAnimationMessage;

	[System.FlagsAttribute]
	public new enum EmotionFlag
	{
		NONE              = 0,
		HAPPY    = 0x01,
		SAD    = 0x10,
		EXCITED    = 0x02,
		BORED    = 0x20,
		ALL = 0xff
	};

	EmotionFlag currentEmotions = EmotionFlag.NONE;
	EmotionFlag lastEmotions = EmotionFlag.NONE;

	public bool HasEmotion( EmotionFlag emo )
	{
		return (currentEmotions | emo) == emo;
	}
	
	public string testAnim = "ANIM_TEST";
	
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
#if UNITY_EDITOR
		if( Input.GetKeyDown(KeyCode.T) )
		{
			SetEmotion(EmotionFlag.HAPPY);
		}

		if( Input.GetKeyDown(KeyCode.Y) )
		{
			SetEmotion(EmotionFlag.HAPPY, true);
		}
#endif
	}

	public void SetEmotion(EmotionFlag emo, bool clear_current = false)
	{
		if( clear_current )
		{
			currentEmotions = emo;
		}
		else
		{
			currentEmotions |= emo;
		}

		if( currentEmotions != lastEmotions || clear_current )
		{
			// send approriate animation
			if (HasEmotion( EmotionFlag.HAPPY) )
			{
				SendAnimation(testAnim, 1);
			}
		}

		lastEmotions = currentEmotions;
	}

	public void SendAnimation(string anim_name, uint num_loops)
	{
		PlayAnimationMessage.animationName = anim_name;
		PlayAnimationMessage.numLoops = num_loops;
		RobotEngineManager.instance.Message.PlayAnimation = PlayAnimationMessage;
		RobotEngineManager.instance.SendMessage();
	}

}
