using UnityEngine;
using System.Collections;
using U2G = Anki.Cozmo.U2G;
using System.Collections.Generic;
using System;

public class CozmoEmotionManager : MonoBehaviour {

	public static CozmoEmotionManager instance = null;
	private U2G.PlayAnimation PlayAnimationMessage;
	CozmoEmotionMachine currentEmotionMachine;

	[System.FlagsAttribute]
	public new enum EmotionFlag
	{
		NONE              = 0,
		HAPPY    = 0x01,
		SAD    = 0x02,
		EXCITED    = 0x04,
		BORED    = 0x08,
		SURPRISED = 0x10,

		ALL = 0xff
	};

	public enum EmotionIntensity
	{
		MILD,
		MODERATE,
		INTENSE
	}

	public List<CozmoAnimation> anims;

	uint currentTarget; 

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
			SetEmotion(EmotionFlag.HAPPY);
		}

		if( Input.GetKeyDown(KeyCode.Y) )
		{
			SetEmotion(EmotionFlag.HAPPY, true);
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
