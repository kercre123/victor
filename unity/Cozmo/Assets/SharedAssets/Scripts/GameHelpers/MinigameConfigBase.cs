using System;
using UnityEngine;

public abstract class MinigameConfigBase : ScriptableObject {

  public abstract int NumCubesRequired();

  public abstract int NumPlayersRequired();

  public GameSkillConfig SkillConfig {
    get { return _SkillConfig; }
  }

  [SerializeField]
  protected GameSkillConfig _SkillConfig;
}



