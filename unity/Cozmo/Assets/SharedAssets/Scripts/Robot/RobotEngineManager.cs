using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

/// <summary>
/// Robot engine manager lives on a GameObject(named MasterObject) in our Intro scene,
/// and handles launching, ticking, and messaging with the Cozmo Engine
/// </summary>
public class RobotEngineManager : MonoBehaviour {
  
  public static RobotEngineManager instance = null;

  public Dictionary<int, Robot> Robots { get; private set; }

  public int CurrentRobotID { get; private set; }

  public Robot CurrentRobot { get { return Robots.ContainsKey(CurrentRobotID) ? Robots[CurrentRobotID] : null; } }

  public bool IsConnected { get { return (channel_ != null && channel_.IsConnected); } }

  public List<string> RobotAnimationNames = new List<string>();

  [SerializeField] 
  private TextAsset configuration_;

  [SerializeField]
  private TextAsset alternateConfiguration_;

  private DisconnectionReason lastDisconnectionReason_ = DisconnectionReason.None;

  public event Action<string> ConnectedToClient;
  public event Action<DisconnectionReason> DisconnectedFromClient;
  public event Action<int> RobotConnected;
  public event Action<bool,RobotActionType> SuccessOrFailure;
  public event Action<bool,string> RobotCompletedAnimation;
  public event Action<bool,uint> RobotCompletedCompoundAction;
  public event Action<bool,uint> RobotCompletedTaggedAction;

  private bool cozmoBindingStarted_ = false;
  private ChannelBase channel_ = null;
  private bool isRobotConnected_ = false;

  private const int UIDeviceID_ = 1;
  private const int UIAdvertisingRegistrationPort_ = 5103;
  // 0 means random unused port
  // Used to be 5106
  private const int UILocalPort_ = 0;

  public bool AllowImageSaving { get; private set; }

  private U2G.MessageGameToEngine message_ = new G2U.MessageGameToEngine();

  public U2G.MessageGameToEngine Message { get { return message_; } }

  private U2G.StartEngine StartEngineMessage = new U2G.StartEngine();
  private U2G.ForceAddRobot ForceAddRobotMessage = new U2G.ForceAddRobot();
  private U2G.ConnectToRobot ConnectToRobotMessage = new U2G.ConnectToRobot();
  private U2G.ConnectToUiDevice ConnectToUiDeviceMessage = new U2G.ConnectToUiDevice();

  private void OnEnable() {
    DAS.Info("RobotEngineManager", "Enabling Robot Engine Manager");
    if (instance != null && instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      instance = this;
      DontDestroyOnLoad(gameObject);
    }

    TextAsset config = configuration_;

    if (PlayerPrefs.GetInt("DebugUseAltConfig", 0) == 1) {
      config = alternateConfiguration_;
    }

    if (config == null) {
      DAS.Error("RobotEngineManager", "Error initializing CozmoBinding: No configuration.");
    }
    else {
      CozmoBinding.Startup(config.text);
      cozmoBindingStarted_ = true;
    }

    Application.runInBackground = true;

    channel_ = new UdpChannel();
    channel_.ConnectedToClient += Connected;
    channel_.DisconnectedFromClient += Disconnected;
    channel_.MessageReceived += ReceivedMessage;

    Robots = new Dictionary<int, Robot>();
  }

  private void OnDisable() {
    
    if (channel_ != null) {
      if (channel_.IsActive) {
        Disconnect();
        Disconnected(DisconnectionReason.UnityReloaded);
      }

      channel_ = null;
    }

    if (cozmoBindingStarted_) {
      CozmoBinding.Shutdown();
    }
  }

  void Update() {
    if (channel_ != null) {
      channel_.Update();
    }
  }

  public void LateUpdate() {
    if (CurrentRobot == null)
      return;

    CurrentRobot.UpdateLightMessages();
  }

  public void AddRobot(byte robotID) {
    if (Robots.ContainsKey(robotID)) {
      Robot oldRobot = Robots[robotID];
      if (oldRobot != null) {
        oldRobot.Dispose();
      }
      Robots.Remove(robotID);
    }
    
    Robot robot = new Robot(robotID);
    Robots.Add(robotID, robot);
    CurrentRobotID = robotID;
  }

  public void Connect(string engineIP) {
    channel_.Connect(UIDeviceID_, UILocalPort_, engineIP, UIAdvertisingRegistrationPort_);
  }

  public void Disconnect() {
    isRobotConnected_ = false;
    if (channel_ != null) {
      channel_.Disconnect();

      // only really needed in editor in case unhitting play
#if UNITY_EDITOR
      float limit = Time.realtimeSinceStartup + 2.0f;
      while (channel_.HasPendingOperations) {
        if (limit < Time.realtimeSinceStartup) {
          DAS.Warn("RobotEngineManager", "Not waiting for disconnect to finish sending.");
          break;
        }
        System.Threading.Thread.Sleep(500);
      }
#endif
    }
  }

  public DisconnectionReason GetLastDisconnectionReason() {
    DisconnectionReason reason = lastDisconnectionReason_;
    lastDisconnectionReason_ = DisconnectionReason.None;
    return reason;
  }

  private void Connected(string connectionIdentifier) {
    if (ConnectedToClient != null) {
      ConnectedToClient(connectionIdentifier);
    }
  }

  private void Disconnected(DisconnectionReason reason) {
    DAS.Debug("RobotEngineManager", "Disconnected: " + reason.ToString());
    isRobotConnected_ = false;

    lastDisconnectionReason_ = reason;
    if (DisconnectedFromClient != null) {
      DisconnectedFromClient(reason);
    }
  }

  public void SendMessage() {
    if (!IsConnected) {
      DAS.Warn("RobotEngineManager", "Message not sent because not connected");
      return;
    }

    channel_.Send(Message);
  }

  private void ReceivedMessage(G2U.MessageEngineToGame message) {
    switch (message.GetTag()) {
    case G2U.MessageEngineToGame.Tag.Ping:
      break;
    case G2U.MessageEngineToGame.Tag.RobotAvailable:
      ReceivedSpecificMessage(message.RobotAvailable);
      break;
    case G2U.MessageEngineToGame.Tag.UiDeviceAvailable:
      ReceivedSpecificMessage(message.UiDeviceAvailable);
      break;
    case G2U.MessageEngineToGame.Tag.RobotConnected:
      ReceivedSpecificMessage(message.RobotConnected);
      break;
    case G2U.MessageEngineToGame.Tag.UiDeviceConnected:
      ReceivedSpecificMessage(message.UiDeviceConnected);
      break;
    case G2U.MessageEngineToGame.Tag.RobotDisconnected:
      ReceivedSpecificMessage(message.RobotDisconnected);
      break;
    case G2U.MessageEngineToGame.Tag.RobotObservedObject:
      ReceivedSpecificMessage(message.RobotObservedObject);
      break;
    case G2U.MessageEngineToGame.Tag.RobotObservedFace:
      ReceivedSpecificMessage(message.RobotObservedFace);
      break;
    case G2U.MessageEngineToGame.Tag.RobotObservedNothing:
      ReceivedSpecificMessage(message.RobotObservedNothing);
      break;
    case G2U.MessageEngineToGame.Tag.DeviceDetectedVisionMarker:
      ReceivedSpecificMessage(message.DeviceDetectedVisionMarker);
      break;
    case G2U.MessageEngineToGame.Tag.PlaySound:
      ReceivedSpecificMessage(message.PlaySound);
      break;
    case G2U.MessageEngineToGame.Tag.StopSound:
      ReceivedSpecificMessage(message.StopSound);
      break;
    case G2U.MessageEngineToGame.Tag.RobotState:
      ReceivedSpecificMessage(message.RobotState);
      break;
    case G2U.MessageEngineToGame.Tag.RobotCompletedAction:
      ReceivedSpecificMessage(message.RobotCompletedAction);
      break;
    case G2U.MessageEngineToGame.Tag.RobotDeletedObject:
      ReceivedSpecificMessage(message.RobotDeletedObject);
      break;
    case G2U.MessageEngineToGame.Tag.RobotMarkedObjectPoseUnknown:
      ReceivedSpecificMessage(message.RobotMarkedObjectPoseUnknown);
      break;
    case G2U.MessageEngineToGame.Tag.ObjectMoved:
      ReceivedSpecificMessage(message.ObjectMoved);
      break;
    case G2U.MessageEngineToGame.Tag.RobotDeletedFace:
      ReceivedSpecificMessage(message.RobotDeletedFace);
      break;
    case G2U.MessageEngineToGame.Tag.ObjectStoppedMoving:
      ReceivedSpecificMessage(message.ObjectStoppedMoving);
      break;
    case G2U.MessageEngineToGame.Tag.ObjectTapped:
      ReceivedSpecificMessage(message.ObjectTapped);
      break;
    case G2U.MessageEngineToGame.Tag.AnimationAvailable:
      ReceivedSpecificMessage(message.AnimationAvailable);
      break;
    case G2U.MessageEngineToGame.Tag.RobotPickedUp:
      break;
    default:
      DAS.Warn("RobotEngineManager", message.GetTag() + " is not supported");
      break;
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotAvailable message) {
    ConnectToRobotMessage.robotID = (byte)message.robotID;

    Message.ConnectToRobot = ConnectToRobotMessage;
    SendMessage();
  }

  private void ReceivedSpecificMessage(G2U.UiDeviceAvailable message) {
    ConnectToUiDeviceMessage.deviceID = (byte)message.deviceID;

    Message.ConnectToUiDevice = ConnectToUiDeviceMessage;
    SendMessage();
  }

  private void ReceivedSpecificMessage(G2U.RobotConnected message) {

  }

  private void ReceivedSpecificMessage(G2U.UiDeviceConnected message) {
    DAS.Debug("RobotEngineManager", "Device connected: " + message.deviceID.ToString());
  }

  private void ReceivedSpecificMessage(G2U.RobotDisconnected message) {
    DAS.Error("RobotEngineManager", "Robot " + message.robotID + " disconnected after " + message.timeSinceLastMsg_sec.ToString("0.00") + " seconds.");
    Disconnect();
    Disconnected(DisconnectionReason.RobotDisconnected);
  }

  private void ReceivedSpecificMessage(ObjectMoved message) {
    if (CurrentRobot == null)
      return;

    int ID = (int)message.objectID;

    if (CurrentRobot.LightCubes.ContainsKey(ID)) {
      LightCube lightCube = CurrentRobot.LightCubes[ID];

      lightCube.Moving(message);
      CurrentRobot.UpdateDirtyList(lightCube);
    }
  }

  private void ReceivedSpecificMessage(ObjectStoppedMoving message) {
    if (CurrentRobot == null)
      return;

    int ID = (int)message.objectID;
    
    if (CurrentRobot.LightCubes.ContainsKey(ID)) {
      LightCube lightCube = CurrentRobot.LightCubes[ID];
      
      lightCube.StoppedMoving(message);
    }
  }

  private void ReceivedSpecificMessage(ObjectTapped message) {
    if (CurrentRobot == null)
      return;
    
    int ID = (int)message.objectID;
    
    if (CurrentRobot.LightCubes.ContainsKey(ID)) {
      LightCube lightCube = CurrentRobot.LightCubes[ID];
      
      lightCube.Tapped(message);
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotObservedObject message) {
    if (CurrentRobot == null)
      return;
    CurrentRobot.UpdateObservedObjectInfo(message);
  }

  private void ReceivedSpecificMessage(G2U.RobotObservedFace message) {
    if (CurrentRobot == null)
      return;
    CurrentRobot.UpdateObservedFaceInfo(message);
  }

  private void ReceivedSpecificMessage(G2U.RobotObservedNothing message) {
    if (CurrentRobot == null)
      return;
    
    if (CurrentRobot.SeenObjects.Count == 0 && !CurrentRobot.isBusy) {
      CurrentRobot.ClearVisibleObjects();
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotDeletedObject message) {
    if (CurrentRobot == null)
      return;

    DAS.Debug("RobotEngineManager", "Deleted object with ID " + message.objectID);

    ObservedObject deleted = CurrentRobot.SeenObjects.Find(x => x == message.objectID);

    CurrentRobot.SeenObjects.Remove(deleted);
    CurrentRobot.VisibleObjects.Remove(deleted);
    CurrentRobot.DirtyObjects.Remove(deleted);
    CurrentRobot.LightCubes.Remove((int)message.objectID);

  }

  private void ReceivedSpecificMessage(G2U.RobotMarkedObjectPoseUnknown message) {
    ObservedObject deleted = CurrentRobot.SeenObjects.Find(x => x == message.objectID);

    CurrentRobot.SeenObjects.Remove(deleted);
    CurrentRobot.VisibleObjects.Remove(deleted);
    CurrentRobot.DirtyObjects.Remove(deleted);

  }

  private void ReceivedSpecificMessage(G2U.RobotDeletedFace message) {
    if (CurrentRobot == null)
      return;
		
    Face deleted = CurrentRobot.Faces.Find(x => x == message.faceID);

    if (deleted != null) {
      DAS.Debug("RobotEngineManager", "Deleted face with ID " + message.faceID);
      CurrentRobot.Faces.Remove(deleted);
    }

  }

  private void ReceivedSpecificMessage(G2U.RobotCompletedAction message) {
    if (CurrentRobot == null)
      return;
    
    RobotActionType actionType = (RobotActionType)message.actionType;
    bool success = (message.result == ActionResult.SUCCESS) || ((actionType == RobotActionType.PLAY_ANIMATION || actionType == RobotActionType.COMPOUND) && message.result == ActionResult.CANCELLED);
    CurrentRobot.targetLockedObject = null;

    CurrentRobot.localBusyTimer = 0f;

    if (SuccessOrFailure != null) {
      SuccessOrFailure(success, actionType);
    }

    if (actionType == RobotActionType.PLAY_ANIMATION) {
      if (RobotCompletedAnimation != null) {
        RobotCompletedAnimation(success, message.completionInfo.animName);
      }
    }

    if (actionType == RobotActionType.COMPOUND) {
      if (RobotCompletedCompoundAction != null) {
        RobotCompletedCompoundAction(success, message.idTag);
      }
    }
    else if (message.idTag > 0) {
      if (RobotCompletedTaggedAction != null) {
        RobotCompletedTaggedAction(success, message.idTag);
      }
    }

    if (!success) {
      if (CurrentRobot.Status(RobotStatusFlag.IS_CARRYING_BLOCK)) {
        CurrentRobot.SetLiftHeight(1f);
      }
      else {
        CurrentRobot.SetLiftHeight(0f);
      }
    }
  }

  private void ReceivedSpecificMessage(G2U.AnimationAvailable message) {
    if (!RobotAnimationNames.Contains(message.animName))
      RobotAnimationNames.Add(message.animName);
  }

  private void ReceivedSpecificMessage(G2U.DeviceDetectedVisionMarker message) {

  }

  private void ReceivedSpecificMessage(G2U.PlaySound message) {
    
  }

  private void ReceivedSpecificMessage(G2U.StopSound message) {
    
  }

  private void ReceivedSpecificMessage(G2U.RobotState message) {
    if (!isRobotConnected_) {
      DAS.Debug("RobotEngineManager", "Robot " + message.robotID.ToString() + " sent first state message.");
      isRobotConnected_ = true;

      AddRobot(message.robotID);

      if (RobotConnected != null) {
        RobotConnected(message.robotID);
      }
    }

    if (!Robots.ContainsKey(message.robotID)) {
      DAS.Debug("RobotEngineManager", "adding robot with ID: " + message.robotID);
      
      AddRobot(message.robotID);
    }
    
    Robots[message.robotID].UpdateInfo(message);
  }

  public void StartEngine(string vizHostIP) {
    StartEngineMessage.asHost = 1;
    int length = 0;
    if (!string.IsNullOrEmpty(vizHostIP)) {
      length = Encoding.UTF8.GetByteCount(vizHostIP);
      if (length + 1 > StartEngineMessage.vizHostIP.Length) {
        throw new ArgumentException("vizHostIP is too long. (" + (length + 1).ToString() + " bytes provided, max " + StartEngineMessage.vizHostIP.Length + ".)");
      }
      Encoding.UTF8.GetBytes(vizHostIP, 0, vizHostIP.Length, StartEngineMessage.vizHostIP, 0);
    }
    StartEngineMessage.vizHostIP[length] = 0;

    Message.StartEngine = StartEngineMessage;
    SendMessage();
  }

  /// <summary>
  /// Forcibly adds a new robot.
  /// </summary>
  /// <param name="robotId">The robot identifier.</param>
  /// <param name="robotIP">The ip address the robot is connected to.</param>
  /// <param name="robotIsSimulated">Specify true for a simulated robot.</param>
  public void ForceAddRobot(int robotID, string robotIP, bool robotIsSimulated) {
    if (robotID < 0 || robotID > 255) {
      throw new ArgumentException("ID must be between 0 and 255.", "robotID");
    }
    
    if (string.IsNullOrEmpty(robotIP)) {
      throw new ArgumentNullException("robotIP");
    }

    if (Encoding.UTF8.GetByteCount(robotIP) + 1 > ForceAddRobotMessage.ipAddress.Length) {
      throw new ArgumentException("IP address too long.", "robotIP");
    }
    int length = Encoding.UTF8.GetBytes(robotIP, 0, robotIP.Length, ForceAddRobotMessage.ipAddress, 0);
    ForceAddRobotMessage.ipAddress[length] = 0;
    
    ForceAddRobotMessage.robotID = (byte)robotID;
    ForceAddRobotMessage.isSimulated = robotIsSimulated ? (byte)1 : (byte)0;

    Message.ForceAddRobot = ForceAddRobotMessage;
    SendMessage();
  }

}
