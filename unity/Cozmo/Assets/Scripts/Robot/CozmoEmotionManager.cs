using UnityEngine;
using System.Collections;
using U2G = Anki.Cozmo.U2G;
using System.Collections.Generic;
using System;

public class CozmoEmotionManager : MonoBehaviour {

	public static CozmoEmotionManager instance = null;
	private U2G.PlayAnimation PlayAnimationMessage;
	CozmoEmotionMachine currentEmotionMachine;
	
	public new enum EmotionType
	{
		NONE,
		IDLE,
		LETS_PLAY,
		PRE_GAME,
		YOUR_TURN,
		SPIN_WHEEL,
		WATCH_SPIN,
		TAP_CUBE,
		WATCH_RESULT,
		EXPECT_REWARD,
		KNOWS_WRONG,
		SHOCKED,
		MINOR_WIN,
		MAJOR_WIN,
		MINOR_FAIL,
		MAJOR_FAIL,
		WIN_MATCH,
		LOSE_MATCH,
		TAUNT
	};

	public enum EmotionIntensity
	{
		MILD,
		MODERATE,
		INTENSE
	}

	uint currentTarget; 

	EmotionType currentEmotions = EmotionType.NONE;
	EmotionType lastEmotions = EmotionType.NONE;

	public bool HasEmotion( EmotionType emo )
	{
		return (currentEmotions | emo) == emo;
	}
	
	public string testAnim = "ANIM_TEST";
	
	void Awake()
	{
		PlayAnimationMessage = new U2G.PlayAnimation();
		instance = this;

		// default machine resides on the master object
		currentEmotionMachine = GetComponent<CozmoEmotionMachine>();
	}

	// Use this for initialization
	void Start () {
	
	}
	
	// Update is called once per frame
	void Update () {
#if UNITY_EDITOR
		if( Input.GetKeyDown(KeyCode.T) )
		{
			SetEmotion(EmotionType.IDLE);
		}

		if( Input.GetKeyDown(KeyCode.Y) )
		{
			SetEmotion(EmotionType.IDLE, true);
		}
#endif

		if( currentEmotionMachine != null )
		{
			currentEmotionMachine.UpdateMachine();
		}
	}

	public void RegisterMachine(CozmoEmotionMachine machine)
	{
		currentEmotionMachine = machine;
	}

	public void UnregisterMachine()
	{
		currentEmotionMachine = null;

		// try to go back to default
		currentEmotionMachine = GetComponent<CozmoEmotionMachine>();
	}

	public void SetEmotion(EmotionType emo, bool clear_current = false)
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
			if (HasEmotion( EmotionType.IDLE) )
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
