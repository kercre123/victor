using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;
using Anki.Cozmo;

namespace AnimationGroups {
  public class AnimationGroupManager : MonoBehaviour {

    public List<AnimationGroup> Groups = new List<AnimationGroup>();

    private static AnimationGroupManager _Instance;

    public static AnimationGroupManager Instance { 
      get { 
        if (_Instance == null) { 
          var go = GameObject.Instantiate(Resources.Load("Prefabs/Managers/AnimationGroupManager") as GameObject);

          go.name = "AnimationGroupManager";
          _Instance = go.GetComponent<AnimationGroupManager>();
          GameObject.DontDestroyOnLoad(go);
        }
        return _Instance; 
      } 
    }

    private void Awake() {
      if (_Instance == null) {
        _Instance = this;
      }

      if (_Instance != this) {
        DAS.Error(this, "Multiple AnimationGroupManager's initialized. Destroying newest one");
        GameObject.Destroy(gameObject);
      }

    }

    public void PlayAnimationFromGroup(Robot robot, string name, Robot.RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
      var group = Groups.Find(x => x.name == name);

      if (group == null) {
        DAS.Error(this, "Cannot find animation group " + name);
        return;
      }

      string animName = group.GetAnimation(robot);

      if (animName == null) {
        DAS.Error(this, "Animation Group " + name + " does not have any animations");
        return;
      }

      robot.SendAnimation(animName, callback, queueActionPosition);
    }
  }
}

