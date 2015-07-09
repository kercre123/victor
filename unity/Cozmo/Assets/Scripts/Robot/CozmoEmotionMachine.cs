using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;

[Serializable]
public struct CozmoAnimation
{
	public string animName;
	public CozmoEmotionManager.EmotionType emotionType;
	public CozmoEmotionManager.EmotionIntensity emotionIntensity;
	
}

[Serializable]
public struct CozmoEmotionState
{
	public string stateName;
	public List<CozmoAnimation> emotionAnims;
	public CozmoEmotionManager.EmotionType emotionType;
	
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
