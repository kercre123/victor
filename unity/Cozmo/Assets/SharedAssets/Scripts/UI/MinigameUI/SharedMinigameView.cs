using UnityEngine;
using Cozmo.UI;
using DG.Tweening;
using System.Collections.Generic;

namespace Cozmo {
  namespace MinigameWidgets {
    public class SharedMinigameView : BaseView {

      public delegate void SharedMinigameViewHandler();

      public event SharedMinigameViewHandler QuitMiniGameConfirmed;
      public event SharedMinigameViewHandler QuitMiniGameViewOpened;
      public event SharedMinigameViewHandler QuitMiniGameViewClosed;

      [SerializeField]
      private GameObject _QuitGameButtonPrefab;

      private QuitMinigameButton _QuitButtonInstance;

      [SerializeField]
      private CozmoStatusWidget _CozmoStatusPrefab;

      private CozmoStatusWidget _CozmoStatusInstance;

      [SerializeField]
      private ChallengeProgressWidget _ChallengeProgressWidgetPrefab;

      private ChallengeProgressWidget _ChallengeProgressWidgetInstance;

      [SerializeField]
      private ChallengeTitleWidget _TitleWidgetPrefab;

      private ChallengeTitleWidget _TitleWidgetInstance;

      private List<IMinigameWidget> _ActiveWidgets = new List<IMinigameWidget>();

      protected override void CleanUp() {
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          widget.DestroyWidgetImmediately();
        }
        _ActiveWidgets.Clear();
      }

      protected override void ConstructOpenAnimation(Sequence openAnimation) {
        Sequence open;
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          open = widget.OpenAnimationSequence();
          if (open != null) {
            openAnimation.Join(open);
          }
        }
      }

      protected override void ConstructCloseAnimation(Sequence closeAnimation) {
        Sequence close;
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          close = widget.CloseAnimationSequence();
          if (close != null) {
            closeAnimation.Join(close);
          }
        }
      }

      public void EnableInteractivity() {
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          widget.EnableInteractivity();
        }
      }

      public void DisableInteractivity() {
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          widget.DisableInteractivity();
        }
      }

      #region Quit Button

      public void CreateQuitButton() {
        if (_QuitButtonInstance != null) {
          return;
        }

        GameObject newButton = UIManager.CreateUIElement(_QuitGameButtonPrefab, this.transform);

        _QuitButtonInstance = newButton.GetComponent<QuitMinigameButton>();

        _QuitButtonInstance.QuitViewOpened += HandleQuitViewOpened;
        _QuitButtonInstance.QuitViewClosed += HandleQuitViewClosed;
        _QuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;

        _ActiveWidgets.Add(_QuitButtonInstance);
      }

      private void HandleQuitViewOpened() {
        if (QuitMiniGameViewOpened != null) {
          QuitMiniGameViewOpened();
        }
      }

      private void HandleQuitViewClosed() {
        if (QuitMiniGameViewClosed != null) {
          QuitMiniGameViewClosed();
        }
      }

      private void HandleQuitConfirmed() {
        if (QuitMiniGameConfirmed != null) {
          QuitMiniGameConfirmed();
        }
      }

      #endregion

      #region StaminaBar

      public void SetMaxCozmoAttempts(int maxAttempts) {
        if (_CozmoStatusInstance != null) {
          _CozmoStatusInstance.SetMaxAttempts(maxAttempts);
        }
        else {
          CreateCozmoStatusWidget(maxAttempts);
          // TODO: Play animation, if dialog had already been opened?
        }
      }

      public void SetCozmoAttemptsLeft(int attemptsLeft) {
        if (_CozmoStatusInstance != null) {
          _CozmoStatusInstance.SetAttemptsLeft(attemptsLeft);
        }
        else {
          CreateCozmoStatusWidget(attemptsLeft);
          // TODO: Play animation, if dialog had already been opened?
        }
      }

      private void CreateCozmoStatusWidget(int attemptsAllowed) {
        if (_CozmoStatusInstance != null) {
          return;
        }

        GameObject statusWidgetObj = UIManager.CreateUIElement(_CozmoStatusPrefab.gameObject, this.transform);
        _CozmoStatusInstance = statusWidgetObj.GetComponent<CozmoStatusWidget>();
        _CozmoStatusInstance.SetMaxAttempts(attemptsAllowed);
        _CozmoStatusInstance.SetAttemptsLeft(attemptsAllowed);
        _ActiveWidgets.Add(_CozmoStatusInstance);
      }

      #endregion

      #region Challenge Progress Widget

      public string ProgressBarLabelText {
        get {
          return _ChallengeProgressWidgetInstance != null ? _ChallengeProgressWidgetInstance.ProgressBarLabelText : null;
        }
        set {
          if (_ChallengeProgressWidgetInstance == null) {
            CreateProgressWidget(value);
          }
          else {
            _ChallengeProgressWidgetInstance.ProgressBarLabelText = value;
          }
        }
      }

      public int NumSegments {
        get {
          return _ChallengeProgressWidgetInstance != null ? _ChallengeProgressWidgetInstance.NumSegments : 1;
        }
        set {
          if (_ChallengeProgressWidgetInstance == null) {
            CreateProgressWidget(null);
          }
          _ChallengeProgressWidgetInstance.NumSegments = value;
        }
      }

      public void SetProgress(float newProgress) {
        if (_ChallengeProgressWidgetInstance == null) {
          CreateProgressWidget(null);
        }
        _ChallengeProgressWidgetInstance.SetProgress(newProgress);
      }

      private void CreateProgressWidget(string progressLabelText = null) {
        if (_ChallengeProgressWidgetInstance != null) {
          return;
        }

        GameObject widgetObj = UIManager.CreateUIElement(_ChallengeProgressWidgetPrefab.gameObject, this.transform);
        _ChallengeProgressWidgetInstance = widgetObj.GetComponent<ChallengeProgressWidget>();

        if (!string.IsNullOrEmpty(progressLabelText)) {
          _ChallengeProgressWidgetInstance.ProgressBarLabelText = progressLabelText;
        }
        _ChallengeProgressWidgetInstance.ResetProgress();

        _ActiveWidgets.Add(_ChallengeProgressWidgetInstance);
      }

      #endregion

      #region Challenge Title Widget

      public string TitleText {
        get {
          return _TitleWidgetInstance != null ? _TitleWidgetInstance.TitleLabelText : null;
        }
        set {
          if (_TitleWidgetInstance != null) {
            _TitleWidgetInstance.TitleLabelText = value;
          }
          else {
            CreateTitleWidget(value);
          }
        }
      }

      private void CreateTitleWidget(string titleText) {
        if (_TitleWidgetInstance != null) {
          return;
        }

        GameObject widgetObj = UIManager.CreateUIElement(_TitleWidgetPrefab.gameObject, this.transform);
        _TitleWidgetInstance = widgetObj.GetComponent<ChallengeTitleWidget>();

        if (!string.IsNullOrEmpty(titleText)) {
          _TitleWidgetInstance.TitleLabelText = titleText;
        }

        _ActiveWidgets.Add(_TitleWidgetInstance);
      }

      #endregion
    }
  }
}