using UnityEngine;
using System.Collections;
using U2G = Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;
using System;

/// <summary>
/// wrapper by which gameplay can trigger basic emote behaviors on the robot
///   default cozmoEmotionMachine is a sibling component
///   emotionMachine can be overwritten for specific modes for minigames or hamster mode, etc
/// </summary>
public class CozmoEmotionManager : MonoBehaviour {
 
  public static CozmoEmotionManager instance = null;
  private U2G.QueueSingleAction QueueSingleAction;
  private U2G.QueueCompoundAction QueueCompoundActionsMessage;
  private U2G.PlayAnimation PlayAnimationMessage;
  private U2G.PlayAnimation[] PlayAnimationMessages;
  private U2G.SetIdleAnimation SetIdleAnimationMessage;
  private U2G.SetLiveIdleAnimationParameters SetLiveIdleAnimationParametersMessage;
  private U2G.GotoPose GotoPoseMessage;
  private U2G.TurnInPlace TurnInPlaceMessage;
  private U2G.FacePose FacePoseMessage;
  CozmoEmotionMachine currentEmotionMachine;

  public enum CompoundActionType {
    ANIM_FACEPOSE = 1,
    ANIM_TURN_IN_PLACE = 2,
    ANIM = 3,
    TURN_IN_PLACE = 4,
    NUM_TYPES
  }

  public enum EmotionType {
    NONE,
    IDLE,
    LETS_PLAY,
    PRE_GAME,
    YOUR_TURN,
    SPIN_WHEEL,
    WATCH_SPIN,
    TAP_CUBE,
    WATCH_RESULT,
    EXPECT_REWARD,
    KNOWS_WRONG,
    SHOCKED,
    MINOR_WIN,
    MAJOR_WIN,
    MINOR_FAIL,
    MAJOR_FAIL,
    WIN_MATCH,
    LOSE_MATCH,
    TAUNT
  }

  public enum EmotionIntensity {
    MILD,
    MODERATE,
    INTENSE
  }

  uint currentTarget;

  EmotionType currentEmotions = EmotionType.NONE;
  EmotionType lastEmotions = EmotionType.NONE;

  protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  public bool HasEmotion(EmotionType emo) {
    return (currentEmotions | emo) == emo;
  }

  public string testAnim = "ANIM_TEST";

  void Awake() {
    QueueSingleAction = new U2G.QueueSingleAction();
    PlayAnimationMessage = new U2G.PlayAnimation();
    PlayAnimationMessages = new U2G.PlayAnimation[2];
    PlayAnimationMessages[0] = new U2G.PlayAnimation();
    PlayAnimationMessages[1] = new U2G.PlayAnimation();

    SetIdleAnimationMessage = new U2G.SetIdleAnimation();
    SetLiveIdleAnimationParametersMessage = new U2G.SetLiveIdleAnimationParameters();
    GotoPoseMessage = new U2G.GotoPose();
    TurnInPlaceMessage = new U2G.TurnInPlace();
    FacePoseMessage = new U2G.FacePose();
    QueueCompoundActionsMessage = new U2G.QueueCompoundAction();
    QueueCompoundActionsMessage.actions = new U2G.RobotActionUnion[2];
    QueueCompoundActionsMessage.actions[0] = new U2G.RobotActionUnion();
    QueueCompoundActionsMessage.actions[1] = new U2G.RobotActionUnion();

    QueueCompoundActionsMessage.actionTypes = new Anki.Cozmo.RobotActionType[2];
    if (instance != null && instance != this) {
      return;
    }
    else {
      instance = this;
      DontDestroyOnLoad(gameObject);
    }

    // default machine resides on the master object
    currentEmotionMachine = GetComponent<CozmoEmotionMachine>();
  }

  // Use this for initialization
  void Start() {
  
  }
  
  // Update is called once per frame
  void Update() {
#if UNITY_EDITOR
    if (Input.GetKeyDown(KeyCode.T)) {
      SetEmotion(testAnim, true);
    }
#endif

    if (currentEmotionMachine != null) {
      currentEmotionMachine.UpdateMachine();
    }
  }

  public void RegisterMachine(CozmoEmotionMachine machine) {
    currentEmotionMachine = machine;
  }

  public void UnregisterMachine() {
    currentEmotionMachine = null;

    // try to go back to default
    currentEmotionMachine = GetComponent<CozmoEmotionMachine>();
  }

  public static string SetEmotion(string emotion_state, bool clear_current = false, string next_emotion_state = "") {
    if (instance == null)
      return null;


    if (instance.currentEmotionMachine != null) {
      // send approriate animation
      if (instance.currentEmotionMachine.HasAnimForState(emotion_state)) {
        string last_anim_name = string.Empty;
        List<CozmoAnimation> anims = instance.currentEmotionMachine.GetAnimsForType(emotion_state);
        CozmoEmotionState emo_state = instance.currentEmotionMachine.GetStateForName(emotion_state);
        if (emo_state.playAllAnims) {
          instance.SendAnimations(anims, clear_current);
        }
        else {
          int rand_index = UnityEngine.Random.Range(0, anims.Count);
          instance.SendAnimation(anims[rand_index], clear_current);
          if (!string.IsNullOrEmpty(next_emotion_state) && instance.currentEmotionMachine.HasAnimForState(next_emotion_state)) {
            List<CozmoAnimation> next_anims = instance.currentEmotionMachine.GetAnimsForType(emotion_state);
            rand_index = UnityEngine.Random.Range(0, next_anims.Count);
            instance.SendAnimation(next_anims[rand_index], false);
          }
        }
        DAS.Debug("CozmoEmotionManager", "setting emotion to " + emotion_state);
        return last_anim_name;
      }
      else {
        DAS.Error("CozmoEmotionManager", "tring to send animation for emotion type " + emotion_state + ", and the current machine has no anim mapped");
      }
    }

    //lastEmotions = currentEmotions;
    return null;
  }

  public void SendAnimation(CozmoAnimation anim, bool stopPreviousAnim = false) {
    if (robot == null)
      return;


    //if (stopPreviousAnim && robot.isBusy && robot.Status(Anki.Cozmo.RobotStatusFlagClad.Animating)) {
    //  robot.CancelAction(Anki.Cozmo.RobotActionType.PLAY_ANIMATION);
    //}
    DAS.Debug("CozmoEmotionManager", "Sending " + anim.animName + " with " + anim.numLoops + " loop" + (anim.numLoops != 1 ? "s" : ""));
    PlayAnimationMessage.animationName = anim.animName;
    PlayAnimationMessage.numLoops = anim.numLoops;
    PlayAnimationMessage.robotID = robot.ID;

    QueueSingleAction.action.playAnimation = PlayAnimationMessage;
    QueueSingleAction.actionType = Anki.Cozmo.RobotActionType.PLAY_ANIMATION;
    QueueSingleAction.numRetries = 0;
    QueueSingleAction.inSlot = 0;
    QueueSingleAction.robotID = robot.ID;

    if (stopPreviousAnim) {
      QueueSingleAction.position = Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING;
    }
    else {
      QueueSingleAction.position = Anki.Cozmo.QueueActionPosition.NEXT;
    }

    RobotEngineManager.instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.instance.SendMessage();
  }

  //FIXME: SendAnimations limited to 2 anims as QueueCompoundActionsMessage is set to 2
  public void SendAnimations(List<CozmoAnimation> anims, bool stopPreviousAnim = false) {
    
    if (robot == null)
      return;
    
    for (int i = 0; i < anims.Count; i++) {
      CozmoAnimation anim = anims[i];
      DAS.Debug("CozmoEmotionManager", "Sending " + anim.animName + " with " + anim.numLoops + " loop" + (anim.numLoops != 1 ? "s" : ""));
      PlayAnimationMessages[i].animationName = anim.animName;
      PlayAnimationMessages[i].numLoops = anim.numLoops;
      PlayAnimationMessages[i].robotID = robot.ID;

      QueueCompoundActionsMessage.actions[i].playAnimation = PlayAnimationMessages[i];
      QueueCompoundActionsMessage.actionTypes[i] = Anki.Cozmo.RobotActionType.PLAY_ANIMATION;
    }

    QueueCompoundActionsMessage.numRetries = 0;
    QueueCompoundActionsMessage.inSlot = 0;
    QueueCompoundActionsMessage.robotID = robot.ID;
    QueueCompoundActionsMessage.parallel = false;

    if (stopPreviousAnim) {
      QueueCompoundActionsMessage.position = Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING;
    }
    else {
      QueueCompoundActionsMessage.position = Anki.Cozmo.QueueActionPosition.NEXT;
    }

    RobotEngineManager.instance.Message.QueueCompoundAction = QueueCompoundActionsMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void SetIdleAnimation(string default_anim) {
    if (robot == null || string.IsNullOrEmpty(default_anim))
      return;

    if (instance.currentEmotionMachine != null) {
      SetIdleAnimationMessage.animationName = default_anim;
      SetIdleAnimationMessage.robotID = robot.ID;
      RobotEngineManager.instance.Message.SetIdleAnimation = SetIdleAnimationMessage;
      RobotEngineManager.instance.SendMessage();
    }
  }

  public void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues) {
    if (robot == null)
      return;

    SetLiveIdleAnimationParametersMessage.paramNames = paramNames;
    SetLiveIdleAnimationParametersMessage.paramValues = paramValues;
    SetLiveIdleAnimationParametersMessage.robotID = 1;

    RobotEngineManager.instance.Message.SetLiveIdleAnimationParameters = SetLiveIdleAnimationParametersMessage;
    RobotEngineManager.instance.SendMessage();
    
  }

  public void SetEmotionReturnToPose(string emotion_state, float x_mm, float y_mm, float rad, bool stopPreviousAnim = false, bool reverse_order = false) {
    if (robot == null)
      return;

    if (instance.currentEmotionMachine != null) {
      // send approriate animation
      if (instance.currentEmotionMachine.HasAnimForState(emotion_state)) {
        List<CozmoAnimation> anims = instance.currentEmotionMachine.GetAnimsForType(emotion_state);
        int rand_index = UnityEngine.Random.Range(0, anims.Count - 1);
        CozmoAnimation anim = anims[rand_index];
        if (stopPreviousAnim && robot.isBusy && robot.Status(Anki.Cozmo.RobotStatusFlag.IS_ANIMATING)) {
          robot.CancelAction(Anki.Cozmo.RobotActionType.PLAY_ANIMATION);
        }
        DAS.Debug("CozmoEmotionManager", "Sending " + anim.animName + " with " + anim.numLoops + " loop" + (anim.numLoops != 1 ? "s" : ""));
        PlayAnimationMessage.animationName = anim.animName;
        PlayAnimationMessage.numLoops = anim.numLoops;
        PlayAnimationMessage.robotID = robot.ID;

        GotoPoseMessage.level = 0;
        GotoPoseMessage.useManualSpeed = 0;
        GotoPoseMessage.x_mm = x_mm;
        GotoPoseMessage.y_mm = y_mm;
        GotoPoseMessage.rad = rad;

        if (!reverse_order) {
          QueueCompoundActionsMessage.actions[0].playAnimation = PlayAnimationMessage;
          QueueCompoundActionsMessage.actionTypes[0] = Anki.Cozmo.RobotActionType.PLAY_ANIMATION;
       
          QueueCompoundActionsMessage.actions[1].goToPose = GotoPoseMessage;
          QueueCompoundActionsMessage.actionTypes[1] = Anki.Cozmo.RobotActionType.DRIVE_TO_POSE;
        }
        else {
          QueueCompoundActionsMessage.actions[0].goToPose = GotoPoseMessage;
          QueueCompoundActionsMessage.actionTypes[0] = Anki.Cozmo.RobotActionType.DRIVE_TO_POSE;

          QueueCompoundActionsMessage.actions[1].playAnimation = PlayAnimationMessage;
          QueueCompoundActionsMessage.actionTypes[1] = Anki.Cozmo.RobotActionType.PLAY_ANIMATION;
        }

        QueueCompoundActionsMessage.numRetries = 0;
        QueueCompoundActionsMessage.inSlot = 0;
        QueueCompoundActionsMessage.robotID = robot.ID;
        QueueCompoundActionsMessage.parallel = false;

        if (stopPreviousAnim) {
          QueueCompoundActionsMessage.position = Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING;
        }
        else {
          QueueCompoundActionsMessage.position = Anki.Cozmo.QueueActionPosition.NEXT;
        }

        RobotEngineManager.instance.Message.QueueCompoundAction = QueueCompoundActionsMessage;
        RobotEngineManager.instance.SendMessage();
      }
      else {
        DAS.Error("CozmoEmotionManager", "tring to send animation for emotion type " + emotion_state + ", and the current machine has no anim mapped");
      }
    }
  }

  public void SetTurnInPlace(float rad, bool stopPreviousAnim = false) {
    if (robot == null)
      return;

    TurnInPlaceMessage.isAbsolute = 1;
    TurnInPlaceMessage.angle_rad = rad;
    TurnInPlaceMessage.robotID = robot.ID;

    QueueSingleAction.action.turnInPlace = TurnInPlaceMessage;
    QueueSingleAction.actionType = Anki.Cozmo.RobotActionType.TURN_IN_PLACE;
    QueueSingleAction.numRetries = 0;
    QueueSingleAction.inSlot = 0;
    QueueSingleAction.idTag = (uint)CompoundActionType.TURN_IN_PLACE;
    QueueSingleAction.robotID = robot.ID;

    if (stopPreviousAnim) {
      QueueSingleAction.position = Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING;
    }
    else {
      QueueSingleAction.position = Anki.Cozmo.QueueActionPosition.NEXT;
    }




    RobotEngineManager.instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.instance.SendMessage();
  }

  public void SetEmotionTurnInPlace(string emotion_state, float rad, bool stopPreviousAction = false, bool reverseOrder = false, bool isParallel = false) {
    if (robot == null)
      return;

    if (instance.currentEmotionMachine != null) {
      // send approriate animation
      if (instance.currentEmotionMachine.HasAnimForState(emotion_state)) {
        List<CozmoAnimation> anims = instance.currentEmotionMachine.GetAnimsForType(emotion_state);
        int rand_index = UnityEngine.Random.Range(0, anims.Count - 1);
        CozmoAnimation anim = anims[rand_index];
        DAS.Debug("CozmoEmotionManager", "Sending " + anim.animName + " with " + anim.numLoops + " loop" + (anim.numLoops != 1 ? "s" : ""));
        PlayAnimationMessage.animationName = anim.animName;
        PlayAnimationMessage.numLoops = anim.numLoops;
        PlayAnimationMessage.robotID = robot.ID;

        TurnInPlaceMessage.isAbsolute = 1;
        TurnInPlaceMessage.angle_rad = rad;
        TurnInPlaceMessage.robotID = robot.ID;

        if (!reverseOrder) {
          QueueCompoundActionsMessage.actions[0].playAnimation = PlayAnimationMessage;
          QueueCompoundActionsMessage.actionTypes[0] = Anki.Cozmo.RobotActionType.PLAY_ANIMATION;

          QueueCompoundActionsMessage.actions[1].turnInPlace = TurnInPlaceMessage;
          QueueCompoundActionsMessage.actionTypes[1] = Anki.Cozmo.RobotActionType.TURN_IN_PLACE;
        }
        else {
          QueueCompoundActionsMessage.actions[0].turnInPlace = TurnInPlaceMessage;
          QueueCompoundActionsMessage.actionTypes[0] = Anki.Cozmo.RobotActionType.TURN_IN_PLACE;

          QueueCompoundActionsMessage.actions[1].playAnimation = PlayAnimationMessage;
          QueueCompoundActionsMessage.actionTypes[1] = Anki.Cozmo.RobotActionType.PLAY_ANIMATION;
        }

        QueueCompoundActionsMessage.numRetries = 0;
        QueueCompoundActionsMessage.inSlot = 0;
        QueueCompoundActionsMessage.robotID = robot.ID;
        QueueCompoundActionsMessage.parallel = isParallel;
        QueueCompoundActionsMessage.idTag = (uint)CompoundActionType.ANIM_TURN_IN_PLACE;

        if (stopPreviousAction) {
          QueueCompoundActionsMessage.position = Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING;
        }
        else {
          QueueCompoundActionsMessage.position = Anki.Cozmo.QueueActionPosition.NEXT;
        }

        RobotEngineManager.instance.Message.QueueCompoundAction = QueueCompoundActionsMessage;
        RobotEngineManager.instance.SendMessage();
      }
      else {
        DAS.Error("CozmoEmotionManager", "tring to send animation for emotion type " + emotion_state + ", and the current machine has no anim mapped");
      }
    }
  }

  public void SetEmotionFacePose(string emotion_state, Face face, bool stopPreviousAction = false, bool reverseOrder = false, bool isParallel = false) {
    if (robot == null)
      return;

    if (instance.currentEmotionMachine != null) {
      // send approriate animation
      if (instance.currentEmotionMachine.HasAnimForState(emotion_state)) {
        List<CozmoAnimation> anims = instance.currentEmotionMachine.GetAnimsForType(emotion_state);
        int rand_index = UnityEngine.Random.Range(0, anims.Count - 1);
        CozmoAnimation anim = anims[rand_index];
        DAS.Debug("CozmoEmotionManager", "Sending " + anim.animName + " with " + anim.numLoops + " loop" + (anim.numLoops != 1 ? "s" : ""));
        PlayAnimationMessage.animationName = anim.animName;
        PlayAnimationMessage.numLoops = anim.numLoops;
        PlayAnimationMessage.robotID = robot.ID;

        FacePoseMessage.maxTurnAngle = float.MaxValue;
        FacePoseMessage.robotID = robot.ID;
        FacePoseMessage.turnAngleTol = Mathf.Deg2Rad; //one degree seems to work?
        FacePoseMessage.world_x = face.WorldPosition.x;
        FacePoseMessage.world_y = face.WorldPosition.y;
        FacePoseMessage.world_z = face.WorldPosition.z;

        if (!reverseOrder) {
          QueueCompoundActionsMessage.actions[0].playAnimation = PlayAnimationMessage;
          QueueCompoundActionsMessage.actionTypes[0] = Anki.Cozmo.RobotActionType.PLAY_ANIMATION;

          QueueCompoundActionsMessage.actions[1].facePose = FacePoseMessage;
          QueueCompoundActionsMessage.actionTypes[1] = Anki.Cozmo.RobotActionType.FACE_POSE;
        }
        else {
          QueueCompoundActionsMessage.actions[0].facePose = FacePoseMessage;
          QueueCompoundActionsMessage.actionTypes[0] = Anki.Cozmo.RobotActionType.FACE_POSE;

          QueueCompoundActionsMessage.actions[1].playAnimation = PlayAnimationMessage;
          QueueCompoundActionsMessage.actionTypes[1] = Anki.Cozmo.RobotActionType.PLAY_ANIMATION;
        }

        QueueCompoundActionsMessage.numRetries = 0;
        QueueCompoundActionsMessage.inSlot = 0;
        QueueCompoundActionsMessage.robotID = robot.ID;
        QueueCompoundActionsMessage.parallel = isParallel;
        QueueCompoundActionsMessage.idTag = (uint)CompoundActionType.ANIM_FACEPOSE;

        if (stopPreviousAction) {
          QueueCompoundActionsMessage.position = Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING;
        }
        else {
          QueueCompoundActionsMessage.position = Anki.Cozmo.QueueActionPosition.NEXT;
        }

        RobotEngineManager.instance.Message.QueueCompoundAction = QueueCompoundActionsMessage;
        RobotEngineManager.instance.SendMessage();
      }
      else {
        DAS.Error("CozmoEmotionManager", "tring to send animation for emotion type " + emotion_state + ", and the current machine has no anim mapped");
      }
    }
  }

  public string GetCurrentDefaultIdle() {
    if (currentEmotionMachine == null) {
      return string.Empty;
    }
    else {
      return currentEmotionMachine.defaultIdleAnimationName;
    }
  }
}
