using UnityEngine;
using System.Collections;

public class CodeBreakerGameConfig : MinigameConfigBase {

}

namespace CodeBreaker {

  public class CodeBreakerGame : GameBase {

    private CodeBreakerReadySlide _ReadySlide;
    private CodeBreakerPanel _GamePanelSlide;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
    }

    protected override void CleanUpOnDestroy() {
    }
  }
}
