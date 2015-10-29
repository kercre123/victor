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
///    and handles launching, ticking, and messaging with the Cozmo Engine
/// </summary>
public class RobotEngineManager : MonoBehaviour {
  
  public static RobotEngineManager instance = null;

  public Dictionary<int, Robot> robots { get; private set; }

  public List<Robot> robotList = new List<Robot>();

  public int currentRobotID { get; private set; }

  public Robot current { get { return robots.ContainsKey(currentRobotID) ? robots[currentRobotID] : null; } }

  public bool IsConnected { get { return (channel != null && channel.IsConnected); } }

  public List<string> robotAnimationNames = new List<string>();

  private float lastTick = 0.0f;
  private const float MAX_ENGINE_TICK_RATE = 0.0667f;

  [SerializeField] private TextAsset configuration;
  [SerializeField] private TextAsset alternateConfiguration;

  [SerializeField]
  [HideInInspector]
  private DisconnectionReason lastDisconnectionReason = DisconnectionReason.None;

  public event Action<string> ConnectedToClient;
  public event Action<DisconnectionReason> DisconnectedFromClient;
  public event Action<int> RobotConnected;
  public event Action<Sprite> RobotImage;
  public event Action<bool,RobotActionType> SuccessOrFailure;
  public event Action<bool,string> RobotCompletedAnimation;
  public event Action<bool,uint> RobotCompletedCompoundAction;
  public event Action<bool,uint> RobotCompletedTaggedAction;

  private bool cozmoBindingStarted = false;
  private ChannelBase channel = null;
  private float lastRobotStateMessage = 0;
  private bool isRobotConnected = false;

  private const int UIDeviceID = 1;
  private const int UIAdvertisingRegistrationPort = 5103;
  // 0 means random unused port
  // Used to be 5106
  private const int UILocalPort = 0;

  public bool AllowImageSaving { get; private set; }

  private bool imageDirectoryCreated = false;

  private const int imageFrameSkip = 0;
  private int imageFrameCount = 0;

  private string imageBasePath;
  private readonly static AsyncCallback EndSave_callback = EndSave;

  private U2G.MessageGameToEngine message = new G2U.MessageGameToEngine();

  public U2G.MessageGameToEngine Message { get { return message; } }

  private U2G.VisualizeQuad VisualizeQuadMessage = new U2G.VisualizeQuad();
  private U2G.StartEngine StartEngineMessage = new U2G.StartEngine();
  private U2G.ForceAddRobot ForceAddRobotMessage = new U2G.ForceAddRobot();
  private U2G.ConnectToRobot ConnectToRobotMessage = new U2G.ConnectToRobot();
  private U2G.ConnectToUiDevice ConnectToUiDeviceMessage = new U2G.ConnectToUiDevice();
  private U2G.SetRobotVolume SetRobotVolumeMessage = new G2U.SetRobotVolume();

  private void OnEnable() {
    if (instance != null && instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      instance = this;
      DontDestroyOnLoad(gameObject);
    }

    TextAsset config = configuration;

    if (PlayerPrefs.GetInt("DebugUseAltConfig", 0) == 1) {
      config = alternateConfiguration;
    }

    if (config == null) {
      DAS.Error("RobotEngineManager", "Error initializing CozmoBinding: No configuration.");
    }
    else {
      CozmoBinding.Startup(config.text);
      cozmoBindingStarted = true;
    }

    Application.runInBackground = true;

    channel = new UdpChannel();
    channel.ConnectedToClient += Connected;
    channel.DisconnectedFromClient += Disconnected;
    channel.MessageReceived += ReceivedMessage;

    robots = new Dictionary<int, Robot>();
  }

  private void OnDisable() {
    
    if (channel != null) {
      if (channel.IsActive) {
        Disconnect();
        Disconnected(DisconnectionReason.UnityReloaded);
      }

      channel = null;
    }

    if (cozmoBindingStarted) {
      CozmoBinding.Shutdown();
    }
  }

  private void FixedUpdate() {
    if (Input.GetKeyDown(KeyCode.Mouse3))
      Debug.Break();

    Profiler.BeginSample("channel.Update");
    if (channel != null) {
      channel.Update();
    }
    Profiler.EndSample();

    Profiler.BeginSample("robots CooldownTimers");
    for (int i = 0; i < robotList.Count; i++) {
      robotList[i].CooldownTimers(Time.deltaTime);
    }
    Profiler.EndSample();

    Profiler.BeginSample("isRobotConnected");
    float robotStateTimeout = 20f;
    if (isRobotConnected && lastRobotStateMessage + robotStateTimeout < Time.realtimeSinceStartup) {
      DAS.Error("RobotEngineManager", "No robot state for " + robotStateTimeout.ToString("0.00") + " seconds.");
      Disconnect();
      Disconnected(DisconnectionReason.RobotConnectionTimedOut);
    }
    Profiler.EndSample();

    if (cozmoBindingStarted) {
      CozmoBinding.Update(Time.realtimeSinceStartup);
    }
  }

  public void LateUpdate() {
    if (current == null)
      return;

    current.UpdateLightMessages();
  }

  public void ToggleVisionRecording(bool on) {
    AllowImageSaving = on;
    
    if (on && !imageDirectoryCreated) {
      
      imageBasePath = Path.Combine(Application.persistentDataPath, DateTime.Now.ToString("robovi\\sion_yyyy-MM-dd_HH-mm-ss"));
      try {
        
        DAS.Debug("RobotEngineManager", "Saving robot screenshots to \"" + imageBasePath + "\"");
        Directory.CreateDirectory(imageBasePath);
        
        imageDirectoryCreated = true;
        
      }
      catch (Exception e) {
        
        AllowImageSaving = false;
        Debug.LogException(e, this);
      }
    }
  }

  public void AddRobot(byte robotID) {
    if (robots.ContainsKey(robotID)) {
      Robot oldRobot = robots[robotID];
      if (oldRobot != null) {
        oldRobot.Dispose();
      }
      robotList.RemoveAll(x => x.ID == robotID);
      robots.Remove(robotID);
    }
    
    
    Robot robot = new Robot(robotID);
    robots.Add(robotID, robot);
    robotList.Add(robot);
    currentRobotID = robotID;
  }

  public void Connect(string engineIP) {
    channel.Connect(UIDeviceID, UILocalPort, engineIP, UIAdvertisingRegistrationPort);
  }

  public void Disconnect() {
    isRobotConnected = false;
    if (channel != null) {
      channel.Disconnect();

      // only really needed in editor in case unhitting play
#if UNITY_EDITOR
      float limit = Time.realtimeSinceStartup + 2.0f;
      while (channel.HasPendingOperations) {
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
    DisconnectionReason reason = lastDisconnectionReason;
    lastDisconnectionReason = DisconnectionReason.None;
    return reason;
  }

  private void Connected(string connectionIdentifier) {
    if (ConnectedToClient != null) {
      ConnectedToClient(connectionIdentifier);
    }

    SetRobotVolume();
  }

  private void Disconnected(DisconnectionReason reason) {
    DAS.Debug("RobotEngineManager", "Disconnected: " + reason.ToString());
    isRobotConnected = false;
    Application.LoadLevel("Shell");

    lastDisconnectionReason = reason;
    if (DisconnectedFromClient != null) {
      DisconnectedFromClient(reason);
    }
  }

  public void SendMessage() {
    if (!IsConnected) {
      DAS.Warn("RobotEngineManager", "Message not sent because not connected");
      return;
    }

    //Debug.Log ("frame("+Time.frameCount+") SendMessage " + Message.GetTag().ToString());
    channel.Send(Message);
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
    // no longer a good indicator
//    if (RobotConnected != null) {
//      RobotConnected((int)message.robotID);
//    }
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
    if (current == null)
      return;

    int ID = (int)message.objectID;

    if (current.activeBlocks.ContainsKey(ID)) {
      ActiveBlock activeBlock = current.activeBlocks[ID];

      activeBlock.Moving(message);
      current.UpdateDirtyList(activeBlock);
    }
  }

  private void ReceivedSpecificMessage(ObjectStoppedMoving message) {
    if (current == null)
      return;

    int ID = (int)message.objectID;
    
    if (current.activeBlocks.ContainsKey(ID)) {
      ActiveBlock activeBlock = current.activeBlocks[ID];
      
      activeBlock.StoppedMoving(message);
    }
  }

  private void ReceivedSpecificMessage(ObjectTapped message) {
    if (current == null)
      return;
    
    int ID = (int)message.objectID;
    
    if (current.activeBlocks.ContainsKey(ID)) {
      ActiveBlock activeBlock = current.activeBlocks[ID];
      
      activeBlock.Tapped(message);
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotObservedObject message) {
    if (current == null)
      return;
    current.UpdateObservedObjectInfo(message);
  }

  private void ReceivedSpecificMessage(G2U.RobotObservedFace message) {
    if (current == null)
      return;
    current.UpdateObservedFaceInfo(message);
  }

  private void ReceivedSpecificMessage(G2U.RobotObservedNothing message) {
    if (current == null)
      return;
    
    if (current.seenObjects.Count == 0 && !current.isBusy) {
      current.ClearVisibleObjects();
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotDeletedObject message) {
    if (current == null)
      return;

    DAS.Debug("RobotEngineManager", "Deleted object with ID " + message.objectID);

    ObservedObject deleted = current.seenObjects.Find(x => x == message.objectID);

    if (deleted != null) {
      deleted.Delete();
      current.seenObjects.Remove(deleted);
    }

    deleted = current.seenObjects.Find(x => x == message.objectID);

    if (deleted != null)
      current.seenObjects.Remove(deleted);

    deleted = current.seenObjects.Find(x => x == message.objectID);
    
    if (deleted != null)
      current.seenObjects.Remove(deleted);

    deleted = current.visibleObjects.Find(x => x == message.objectID);
    
    if (deleted != null)
      current.visibleObjects.Remove(deleted);

    if (current.activeBlocks.ContainsKey((int)message.objectID))
      current.activeBlocks.Remove((int)message.objectID);
  }

  private void ReceivedSpecificMessage(G2U.RobotMarkedObjectPoseUnknown message) {
    ObservedObject deleted = current.seenObjects.Find(x => x == message.objectID);

    if (deleted != null) {
      current.seenObjects.Remove(deleted);
    }

    deleted = current.visibleObjects.Find(x => x == message.objectID);
    if (deleted != null) {
      current.visibleObjects.Remove(deleted);
    }

    deleted = current.dirtyObjects.Find(x => x == message.objectID);
    if (deleted != null) {
      current.dirtyObjects.Remove(deleted);
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotDeletedFace message) {
    if (current == null)
      return;
		
    Face deleted = current.faceObjects.Find(x => x == message.faceID);

    if (deleted != null) {
      DAS.Debug("RobotEngineManager", "Deleted face with ID " + message.faceID);
      current.faceObjects.Remove(deleted);
    }

  }

  private void ReceivedSpecificMessage(G2U.RobotCompletedAction message) {
    if (current == null)
      return;
    
    RobotActionType action_type = (RobotActionType)message.actionType;
    bool success = (message.result == ActionResult.SUCCESS) || ((action_type == RobotActionType.PLAY_ANIMATION || action_type == RobotActionType.COMPOUND) && message.result == ActionResult.CANCELLED);
    current.targetLockedObject = null;

    current.localBusyTimer = 0f;

    if (SuccessOrFailure != null) {
      SuccessOrFailure(success, action_type);
    }

    if (action_type == RobotActionType.PLAY_ANIMATION) {
      if (RobotCompletedAnimation != null) {
        RobotCompletedAnimation(success, message.completionInfo.animName);
      }
    }

    if (action_type == RobotActionType.COMPOUND) {
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
      if (current.Status(RobotStatusFlag.IS_CARRYING_BLOCK)) {
        current.SetLiftHeight(1f);
      }
      else {
        current.SetLiftHeight(0f);
      }
    }
  }

  private void ReceivedSpecificMessage(G2U.AnimationAvailable message) {
    if (!robotAnimationNames.Contains(message.animName))
      robotAnimationNames.Add(message.animName);
  }

  private void ReceivedSpecificMessage(G2U.DeviceDetectedVisionMarker message) {

  }

  private void ReceivedSpecificMessage(G2U.PlaySound message) {
    
  }

  private void ReceivedSpecificMessage(G2U.StopSound message) {
    
  }

  private void ReceivedSpecificMessage(G2U.RobotState message) {
    if (!isRobotConnected) {
      DAS.Debug("RobotEngineManager", "Robot " + message.robotID.ToString() + " sent first state message.");
      isRobotConnected = true;

      AddRobot(message.robotID);

      if (RobotConnected != null) {
        RobotConnected(message.robotID);
      }
    }

    lastRobotStateMessage = Time.realtimeSinceStartup;

    if (!robots.ContainsKey(message.robotID)) {
      DAS.Debug("RobotEngineManager", "adding robot with ID: " + message.robotID);
      
      AddRobot(message.robotID);
    }
    
    robots[message.robotID].UpdateInfo(message);
  }

  private Texture2D texture;
  private TextureFormat textureFormat;
  private Sprite sprite;
  private Vector2 spritePivot = new Vector2(.5f, .5f);
  private int currentImageIndex;
  private int currentChunkIndex;
  private UInt32 currentImageID = UInt32.MaxValue;
  private UInt32 currentImageFrameTimeStamp = UInt32.MaxValue;
  private Color32[] color32Array;
  private byte[] jpegArray;
  private byte[] minipegArray;

  // Pre-baked JPEG header for grayscale, Q50
  private static readonly byte[] minipeg_header50 = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, // 0x19 = QTable
    0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23,
    0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40,
    0x44, 0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D,
    
    //0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0xF0, // 0x5E = Height x Width
    0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x01, 0x28, // 0x5E = Height x Width
    
    //0x01, 0x40, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
    0x01, 0x90, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
    
    0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
    0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
    0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
    0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
    0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01,
    0x00, 0x00, 0x3F, 0x00
  };
  
  // Pre-baked JPEG header for grayscale, Q80
  private static readonly byte[] minipeg_header80 = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x06, 0x04, 0x05, 0x06, 0x05, 0x04, 0x06,
    0x06, 0x05, 0x06, 0x07, 0x07, 0x06, 0x08, 0x0A, 0x10, 0x0A, 0x0A, 0x09, 0x09, 0x0A, 0x14, 0x0E,
    0x0F, 0x0C, 0x10, 0x17, 0x14, 0x18, 0x18, 0x17, 0x14, 0x16, 0x16, 0x1A, 0x1D, 0x25, 0x1F, 0x1A,
    0x1B, 0x23, 0x1C, 0x16, 0x16, 0x20, 0x2C, 0x20, 0x23, 0x26, 0x27, 0x29, 0x2A, 0x29, 0x19, 0x1F,
    0x2D, 0x30, 0x2D, 0x28, 0x30, 0x25, 0x28, 0x29, 0x28, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0xF0,
    0x01, 0x40, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
    0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
    0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
    0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
    0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01,
    0x00, 0x00, 0x3F, 0x00
  };

  public void SaveAsync(string filepath, byte[] buffer, int start, int length) {
    FileStream stream = new FileStream(filepath, FileMode.Create, FileAccess.Write, FileShare.None, 0x1000, true);
    try {
      stream.BeginWrite(buffer, start, length, EndSave_callback, stream);
    }
    catch (Exception e) {
      Debug.LogException(e, this);
    }
  }

  private static void EndSave(IAsyncResult result) {
    try {
      FileStream stream = (FileStream)result.AsyncState;
      stream.EndWrite(result);
    }
    catch (Exception e) {
      Debug.LogException(e);
    }
  }

  public void VisualizeQuad(uint ID, uint color, Vector3 upperLeft, Vector3 upperRight, Vector3 lowerRight, Vector3 lowerLeft) {

    VisualizeQuadMessage.color = color;
    VisualizeQuadMessage.quadID = ID;

    VisualizeQuadMessage.xUpperLeft = upperLeft.x;
    VisualizeQuadMessage.xUpperRight = upperRight.x;
    VisualizeQuadMessage.xLowerLeft = lowerLeft.x;
    VisualizeQuadMessage.xLowerRight = lowerRight.x;

    VisualizeQuadMessage.yUpperLeft = upperLeft.y;
    VisualizeQuadMessage.yUpperRight = upperRight.y;
    VisualizeQuadMessage.yLowerLeft = lowerLeft.y;
    VisualizeQuadMessage.yLowerRight = lowerRight.y;

    VisualizeQuadMessage.zUpperLeft = upperLeft.z;
    VisualizeQuadMessage.zUpperRight = upperRight.z;
    VisualizeQuadMessage.zLowerLeft = lowerLeft.z;
    VisualizeQuadMessage.zLowerRight = lowerRight.z;

    Message.VisualizeQuad = VisualizeQuadMessage;
    SendMessage();
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

  public void SetRobotVolume() {
    SetRobotVolumeMessage.volume = 0.0f;
    DAS.Debug("RobotEngineManager", "Set Robot Volume " + SetRobotVolumeMessage.volume);

    Message.SetRobotVolume = SetRobotVolumeMessage;
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
