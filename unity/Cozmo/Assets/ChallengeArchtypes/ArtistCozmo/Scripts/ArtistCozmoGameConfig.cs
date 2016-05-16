using UnityEngine;
using System.Collections;

public class ArtistCozmoGameConfig : MinigameConfigBase {

  public Gradient[] ColorGradients;

  [Range(1,8)]
  public int MaxColorBits = 8;

  [Range(1,8)]
  public int MinColorBits = 1;

  #region implemented abstract members of MinigameConfigBase
  public override int NumCubesRequired() {
    return 0;
  }
  public override int NumPlayersRequired() {
    return 1;
  }
  #endregion
}
