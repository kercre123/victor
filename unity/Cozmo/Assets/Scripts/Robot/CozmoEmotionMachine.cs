using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;

[Serializable]
public struct CozmoAnimation
{
	public string animName;
	public uint numLoops;
	
}

[Serializable]
public struct CozmoEmotionState
{
	//public string stateName;
	public CozmoEmotionManager.EmotionType emotionType;
	public List<CozmoAnimation> emotionAnims;
	
}

public class CozmoEmotionMachine : MonoBehaviour {

	public CozmoEmotionState defaultEmotionState;
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

	public void OnEnable()
	{
		StartMachine();
	}
	
	
	public void OnDisable()
	{
		CloseMachine();
	}

	// initailization
	public void StartMachine()
	{
		if( CozmoEmotionManager.instance != null )
		{
			CozmoEmotionManager.instance.RegisterMachine(this);
			CozmoEmotionManager.SetEmotion(defaultEmotionState.emotionType);
		}
		InitializeMachine();
	}

	public virtual void InitializeMachine() {}

	public virtual void UpdateMachine(){}
	
	// clean-up
	public void CloseMachine()
	{
		if( CozmoEmotionManager.instance != null )
		{
			CozmoEmotionManager.instance.UnregisterMachine();
		}
		CleanUp();
	}

	public virtual void CleanUp(){}

	public bool HasAnimForType(CozmoEmotionManager.EmotionType emotionType)
	{
		if( typeAnims.ContainsKey(emotionType) && typeAnims[emotionType].Count > 0 )
			return true;
		return false;
	}

	public CozmoAnimation GetAnimForType(CozmoEmotionManager.EmotionType emotionType)
	{
		// note: do not call with out first calling too HasAnimForType
		int rand_index = UnityEngine.Random.Range(0, typeAnims[emotionType].Count - 1);
		return typeAnims[emotionType][rand_index];
	}
	
}
