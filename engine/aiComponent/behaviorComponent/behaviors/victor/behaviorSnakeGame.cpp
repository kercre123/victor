/**
 * File: BehaviorSnakeGame.cpp
 *
 * Author: ross
 * Created: 2018-02-27
 *
 * Description: victor plays the game snake. his solver is greedy, so it eventually fails
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorSnakeGame.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/common/engine/colorRGBA.h"
#include "coretech/vision/engine/image.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/components/animationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/victor/snakeGame.h"
#include "engine/aiComponent/behaviorComponent/behaviors/victor/snakeGameSolver.h"


namespace Anki {
namespace Vector {
  
namespace {
  unsigned int kTicksPerGameUpdate = 1;
  unsigned int kScalingFactor = 2*2; // must be power of 2
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSnakeGame::BehaviorSnakeGame(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSnakeGame::~BehaviorSnakeGame()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSnakeGame::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSnakeGame::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSnakeGame::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSnakeGame::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // move head up
  auto* action = new MoveHeadToAngleAction( M_PI/4 );
  DelegateIfInControl(action, [&](ActionResult result) {
    auto& rng = GetBEI().GetRNG();
    
    unsigned int initLength = 10;
    
    unsigned int snakeWidth = (FACE_DISPLAY_WIDTH - 2*kScalingFactor)/kScalingFactor;
    unsigned int snakeHeight = (FACE_DISPLAY_HEIGHT - 2*kScalingFactor)/kScalingFactor;
    
    _dVars.image.reset( new Vision::Image( FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH, NamedColors::BLACK ) );
    _dVars.game.reset( new SnakeGame( snakeWidth, snakeHeight, initLength, rng ) );
    const float probMistakePerLength = 0.001f;
    _dVars.solver.reset( new SnakeGameSolver( *_dVars.game.get(), 0.0f, probMistakePerLength, 0.0f, rng ) );
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSnakeGame::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  if( _dVars.image == nullptr ) {
    // not finished with head motion yet
    return;
  }
  if( _dVars.lost ) {
    return;
  }
  
  auto& image = *_dVars.image.get();
  
  ++_dVars.gameTicks;
  if( _dVars.gameTicks >= kTicksPerGameUpdate ) {
    _dVars.gameTicks = 0;
  
    // update game and solver
    _dVars.game->Update();
    
    if( _dVars.game->GameOver() ) {
      _dVars.lost = true;
      CancelDelegates(false);
      // lost. play an animation and end
      auto* newAction = new TriggerLiftSafeAnimationAction( AnimationTrigger::AudioOnlyHuh );
      DelegateIfInControl(newAction, [this](ActionResult result) {
        CancelSelf();
      });
      return;
    }
    
    auto oldDirection = _dVars.game->GetDirection();
    _dVars.solver->ChooseAndApplyMove();
    auto newDirection = _dVars.game->GetDirection();
    
    if( oldDirection != newDirection ) {
      AnimateDirection( static_cast<uint8_t>(newDirection) );
    }
    
    // clear face
    image = Vision::Image( FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH, NamedColors::BLACK );
  
    // draw face
    DrawGame( image );
  }
  
  GetBEI().GetAnimationComponent().DisplayFaceImage( image, 1.0f, true );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSnakeGame::DrawGame( Vision::Image& image ) const
{
  if( !ANKI_VERIFY(_dVars.game != nullptr,
                   "",
                   "") )
  {
    return;
  }
  
  const auto& game = *_dVars.game.get();
  
  int height = image.GetNumRows();
  int width = image.GetNumCols();
  if( !ANKI_VERIFY( (width == kScalingFactor*(2+game.GetWidth())) && (height == kScalingFactor*(2+game.GetHeight())) ,
                    "",
                    "" ) )
  {
    return;
  }
  
  
  auto* pImg = image.GetRawDataPointer();
  
  auto setImg = [pImg,width](int i, int j, u8 value) {
    *(pImg + (j*width) + i) = value;
  };
  
  
  
  for( int j=0; j<height; ++j ) {
    for( int i=0; i<width; ++i ) {
      unsigned int snakeI = (i-1)/kScalingFactor;
      unsigned int snakeJ = (j-1)/kScalingFactor;
      if( i<kScalingFactor || j<kScalingFactor || i>=width-kScalingFactor || j>=height-kScalingFactor ) {
        setImg(i,j, 255);
        continue;
      }
      if( game.GetSnake().IsSnakeAt(snakeI, snakeJ) ) {
        setImg(i,j, 255);
      } else if( game.GetFood().x == snakeI && game.GetFood().y == snakeJ ) {
        setImg(i,j, 255);
      }
    }
  }
}
  
void BehaviorSnakeGame::AnimateDirection( uint8_t directionInt )
{
  auto direction = static_cast<SnakeGame::Direction>(directionInt);
  if( (direction == SnakeGame::Direction::LEFT)
      || (direction == SnakeGame::Direction::RIGHT) )
  {
    float degrees = 0.0f;
    // note: this is inverted... screen right is victor left
    if( direction == SnakeGame::Direction::LEFT ) {
      degrees = -10.f;
    } else {
      degrees = 10.f;
    }
    auto turnAction = new TurnInPlaceAction( DEG_TO_RAD(degrees), false );
    turnAction->SetAccel(MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2);
    turnAction->SetMaxSpeed(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);
    CancelDelegates();
    DelegateIfInControl(turnAction);
  } else {
    IAction* liftAction;
    if( direction == SnakeGame::Direction::UP ) {
      liftAction = new MoveLiftToHeightAction(10.0f); // not working
    } else {
      liftAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
    }
    CancelDelegates();
    DelegateIfInControl(liftAction);
  }
}
  
}
}
