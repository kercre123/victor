using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  // This doesn't use a standard state machine so that data can
  // Be added in editor
  public class OnboardingBaseStage : MonoBehaviour {

    public bool ActiveTopBar { get { return _ActiveTopBar; } }
    public bool ActiveTabButtons { get { return _ActiveTabButtons; } }
    public bool ActiveMenuContent { get { return _ActiveMenuContent; } }

    [SerializeField]
    protected bool _ActiveTopBar = false;

    [SerializeField]
    protected bool _ActiveTabButtons = false;

    [SerializeField]
    protected bool _ActiveMenuContent = false;


    public virtual void Start() {
      DAS.Info("onboarding.stage.started", name);
    }

    public virtual void SkipPressed() {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
