using System;
using System.Collections;
using Anki.Cozmo;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Needs.UI {
  public class PlayTokenRewardModal : BaseModal {

    [SerializeField]
    private CozmoButton _ContinueButton;

    [SerializeField]
    private GameObject _PlayTokenAwardedEffectPrefab;

    // Used when the animation repeats - this prefab only has the shape burst,
    // not the icon 'dropping in' from above or the motes
    [SerializeField]
    private GameObject _PlayTokenAwardedEffectNoIconPrefab;

    [SerializeField]
    private RectTransform _PlayTokenEffectAnchor;

    [SerializeField]
    private int _BurstEffectLoopTime = 22;

    public void Start() {
      _ContinueButton.Initialize(HandleContinueButtonClicked, "play_token_reward_continue_button", DASEventDialogName);
      // PlayTokenAwardedEffectPrefab has a DestroySelf script, so it will clean itself up after it finishes playing
      GameObject.Instantiate(_PlayTokenAwardedEffectPrefab, _PlayTokenEffectAnchor);

      StartCoroutine(LoopEffect());
    }

    private void HandleContinueButtonClicked() {
      UIManager.CloseModal(this);
    }

    private IEnumerator LoopEffect() {
      while (enabled) {
        yield return new WaitForSeconds(_BurstEffectLoopTime);
        // PlayTokenAwardedEffectNoIconPrefab has a DestroySelf script, so it will clean itself up after it finishes playing
        GameObject.Instantiate(_PlayTokenAwardedEffectNoIconPrefab, _PlayTokenEffectAnchor);
      }
    }

  }
}
