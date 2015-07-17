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
	public string stateName;
	public List<CozmoAnimation> emotionAnims;
	
}

public class CozmoEmotionMachine : MonoBehaviour {

	public string defaultEmotionState;
	public List<CozmoEmotionState> emotionStates;
	private Dictionary<string, List<CozmoAnimation>> typeAnims = new Dictionary<string, List<CozmoAnimation>>();

	public void Awake()
	{
		// populate our helper look up
		for(int i = 0; i < emotionStates.Count; i++)
		{
			if( string.IsNullOrEmpty(emotionStates[i].stateName) )
			{
				Debug.LogError("trying to add state with no name");
			}
			else
			{
				if( !typeAnims.ContainsKey(emotionStates[i].stateName) )
				{
					typeAnims.Add(emotionStates[i].stateName, emotionStates[i].emotionAnims);
				}
				else
				{
					Debug.LogError("trying to add " + emotionStates[i].stateName + " more than once");
				}
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
			CozmoEmotionManager.SetEmotion(defaultEmotionState);
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

	public bool HasAnimForState(string state_name)
	{
		if( typeAnims.ContainsKey(state_name) && typeAnims[state_name].Count > 0 )
			return true;
		return false;
	}

	public CozmoAnimation GetAnimForType(string state_name)
	{
		// note: do not call with out first calling too HasAnimForType
		int rand_index = UnityEngine.Random.Range(0, typeAnims[state_name].Count - 1);
		return typeAnims[state_name][rand_index];
	}
	
}
