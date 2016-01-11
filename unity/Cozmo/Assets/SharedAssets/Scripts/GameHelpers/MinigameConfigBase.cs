using System;
using UnityEngine;

public abstract class MinigameConfigBase : ScriptableObject {
  public abstract int NumCubesRequired();

  public abstract int NumPlayersRequired();
}



