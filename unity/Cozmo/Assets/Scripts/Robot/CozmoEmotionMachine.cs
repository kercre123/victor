using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;

[Serializable]
public struct CozmoAnimation
{
	public string animName;
	public int numLoops;
	
}

[Serializable]
public struct CozmoEmotionState
{
	public string stateName;
	public CozmoEmotionManager.EmotionType emotionType;
	public List<CozmoAnimation> emotionAnims;
	
}

public class CozmoEmotionMachine : MonoBehaviour {

	public string defaultEmotionStateName;
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
		CleanUp();
	}

	public virtual void CleanUp(){}
	
}
