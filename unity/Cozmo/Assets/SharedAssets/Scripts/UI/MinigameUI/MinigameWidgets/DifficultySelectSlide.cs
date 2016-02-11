using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using System;
using Cozmo.MinigameWidgets;
using DG.Tweening;

public class DifficultySelectSlide : MonoBehaviour, IMinigameWidget {

  [SerializeField]
  private DifficultySelectOption _OptionPrefab;

  [SerializeField]
  private RectTransform _OptionTray;

  [SerializeField]
  private AnkiTextLabel _DescriptionLabel;

  private DifficultySelectOptionData _SelectedDifficulty;

  private readonly List<DifficultySelectOption> _SpawnedOptions = new List<DifficultySelectOption>();

  public DifficultySelectOptionData SelectedDifficulty {
    get {
      return _SelectedDifficulty;
    }
  }

  private void ClearSpawnedOptions() {
    for (int i = 0; i < _SpawnedOptions.Count; i++) {
      Destroy(_SpawnedOptions[i].gameObject);
    }
    _SpawnedOptions.Clear();
  }

  private void HandleDifficultySelected(DifficultySelectOption selectedDifficultyOption) {
    _SelectedDifficulty = selectedDifficultyOption.Data;
    for (int i = 0; i < _SpawnedOptions.Count; i++) {
      _SpawnedOptions[i].IsSelected = (_SpawnedOptions[i] == selectedDifficultyOption);
    }
    _DescriptionLabel.text = _SelectedDifficulty.DifficultyDescription;

  }


  public void Initialize(List<DifficultySelectOptionData> options, int highestDifficultyAvailable) {
    for (int i = 0; i < options.Count; i++) {
      var obj = UIManager.CreateUIElement(_OptionPrefab, _OptionTray);

      var option = obj.GetComponent<DifficultySelectOption>();
      option.Initialize(options[i], enabled: options[i].DifficultyId <= highestDifficultyAvailable);
      option.OnSelect += HandleDifficultySelected; 
      _SpawnedOptions.Add(option);
    }

    _SpawnedOptions[0].IsSelected = true;
  }



  #region IMinigameWidget implementation

  public void DestroyWidgetImmediately() {
    Destroy(gameObject);
  }

  // TODO: Don't hardcode this
  public Sequence OpenAnimationSequence() {
    Sequence open = DOTween.Sequence();
    open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 200, 
      this.transform.localPosition.y + 200, this.transform.localPosition.z),
      0.25f).From().SetEase(Ease.OutQuad));
    return open;
  }

  // TODO: Don't hardcode this
  public Sequence CloseAnimationSequence() {
    Sequence close = DOTween.Sequence();
    close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 200, 
      this.transform.localPosition.y + 200, this.transform.localPosition.z),
      0.25f).SetEase(Ease.OutQuad));
    return close;
  }

  #endregion
}
