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
	//public string stateName;
	public CozmoEmotionManager.EmotionType emotionType;
	public List<CozmoAnimation> emotionAnims;
	
}

public class CozmoEmotionMachine : MonoBehaviour {

	public string defaultEmotionStateName;
	public List<CozmoEmotionState> emotionStates;
	private Dictionary<CozmoEmotionManager.EmotionType, List<CozmoAnimation>> typeAnims = new Dictionary<CozmoEmotionManager.EmotionType, List<CozmoAnimation>>();

	public void Awake()
	{
		// populate our helper look up
		for(int i = 0; i < emotionStates.Count; i++)
		{
			if( !typeAnims.ContainsKey(emotionStates[i].emotionType) )
			{
				typeAnims.Add(emotionStates[i].emotionType, emotionStates[i].emotionAnims);
			}
			else
			{
				Debug.LogError("trying to add " + emotionStates[i].emotionType + " more than once");
			}
		}

	}

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

	public CozmoAnimation GetAnimForType(CozmoEmotionManager.EmotionType emotionType)
	{
		if( typeAnims.ContainsKey(emotionType) && typeAnims[emotionType].Count > 0 )
		{
			int rand_index = UnityEngine.Random.Range(0, typeAnims[emotionType].Count - 1);
			return typeAnims[emotionType][rand_index];
		}
		else
		{
			Debug.LogError("No anims found for " + emotionType);
			return null;
		}
	}
	
}
