/**
 * File: BehaviorBlackJack.h
 *
 * Author: Sam Russell
 * Created: 2018-05-09
 *
 * Description: BlackJack
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBlackJack__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBlackJack__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/blackjack/blackJackSimulation.h"
#include "engine/aiComponent/behaviorComponent/behaviors/blackjack/blackJackVisualizer.h"

namespace Anki {
namespace Vector {

// Fwd Declarations
class BehaviorPromptUserForVoiceCommand;
class BehaviorTextToSpeechLoop;
class BehaviorLookAtFaceInFront;

class BehaviorBlackJack : public ICozmoBehavior
{
public: 
  virtual ~BehaviorBlackJack();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorBlackJack(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:

  enum class EState {
    TurnToFace,
    GetIn,
    Dealing,
    HitOrStandPrompt,
    HitOrStand,
    ReactToPlayerCard,
    VictorsTurn,
    DealToVictor,
    ReactToDealerCard,
    EndGame,
    PlayAgainPrompt,
    PlayAgain,
    GetOut
  };

  enum class EDealingState {
    PlayerFirstCard,
    DealerFirstCard,
    PlayerSecondCard,
    DealerSecondCard,
    Finished
  };

  enum class EOutcome{
    Tie,
    VictorWins,
    VictorWinsBlackJack,
    VictorLoses,
    VictorBusts,
    VictorLosesBlackJack
  };

  void TransitionToTurnToFace();
  void TransitionToGetIn();
  void TransitionToDealing();
  void TransitionToReactToPlayerCard();
  void TransitionToHitOrStandPrompt();
  void TransitionToHitOrStand();
  void TransitionToVictorsTurn();
  void TransitionToDealToVictor();
  void TransitionToReactToDealerCard();
  void TransitionToEndGame();
  void TransitionToPlayAgainPrompt();
  void TransitionToPlayAgain();
  void TransitionToGetOut();

  struct InstanceConfig {
    InstanceConfig();
    std::shared_ptr<BehaviorPromptUserForVoiceCommand> hitOrStandPromptBehavior;
    std::shared_ptr<BehaviorPromptUserForVoiceCommand> playAgainPromptBehavior;
    std::shared_ptr<BehaviorTextToSpeechLoop>          ttsBehavior;
    std::shared_ptr<BehaviorTextToSpeechLoop>          goodLuckTTSBehavior;
    std::shared_ptr<BehaviorLookAtFaceInFront>         lookAtFaceInFrontBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    EState              state;
    EDealingState       dealingState;
    EOutcome            outcome;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  BlackJackGame _game;
  BlackJackVisualizer _visualizer;

  // Helper functions
  IBehavior* SetUpSpeakingBehavior(const std::string& vocalizationString);
  void SetState_internal(EState state, const std::string& stateName);
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBlackJack__
