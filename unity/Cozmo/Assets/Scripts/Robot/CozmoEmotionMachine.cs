using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;

[Serializable]
public struct CozmoAnimation
{
	public string animName;
	public CozmoEmotionManager.EmotionFlag emotionType;
	public CozmoEmotionManager.EmotionIntensity emotionIntensity;
	
}

[Serializable]
public struct CozmoEmotionState
{
	public string stateName;
	public CozmoEmotionManager.EmotionFlag emotionType;
	public CozmoEmotionManager.EmotionIntensity emotionIntensity;
	public List<CozmoEmotionManager.EmotionFlag> incrementEmotions;
	public List<CozmoEmotionManager.EmotionFlag> decrementEmotions;
	
}

public class CozmoEmotionMachine : MonoBehaviour {

	public List<CozmoEmotionState> emotionStates;

	// initailization
	public void StartMachine()
	{
		CozmoEmotionManager.instance.RegisterMachine(this);
		InitializeMachine();
	}

	public virtual void InitializeMachine() {}

	public virtual void UpdateMachine(){}
	
	// clean-up
	public void CloseMachine()
	{
		CozmoEmotionManager.instance.UnregisterMachine();
	}

	public virtual void CleanUp(){}
	
}
