using UnityEngine;
using System.Collections;

public class FriendshipFormulaConfiguration : ScriptableObject {

	[SerializeField]
	private float[] _Multipliers = new float[(int)Anki.Cozmo.ProgressionStatType.Count];


	public float CalculateFriendshipScore(StatContainer stats) {
		float total = 0f;
		for (int i = 0; i < _Multipliers.Length; i++) {
			total += _Multipliers[i] * stats[(Anki.Cozmo.ProgressionStatType)i];
		}
		return total;
	}
}
