using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class CozmoBusyPanel : MonoBehaviour {

  [SerializeField] Text text_actionDescription;
  [SerializeField] GameObject panel;
  [SerializeField] AudioClip affirmativeSound;

  Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  public static CozmoBusyPanel instance = null;

  float timer = 0f;
  float soundDelay = 0.25f;
  bool soundPlayed = false;

  bool mute = false;

  void Awake() {
    //enforce singleton
//    if(instance != null && instance != this) {
//      GameObject.Destroy(instance.gameObject);
//      return;
//    }
    instance = this;
    //DontDestroyOnLoad(gameObject);
  }

  void OnEnable() {
    timer = soundDelay;
  }

  // Update is called once per frame
  void Update () {
    if(robot == null || !robot.isBusy) {
      panel.SetActive(false);
      timer = soundDelay;
      return;
    }

    if(timer > 0f) {
      timer -= Time.deltaTime;

      if(!mute && timer <= 0f && affirmativeSound != null) {
        AudioManager.PlayOneShot(affirmativeSound);
      }
    }

    panel.SetActive(true);
  }

  public void CancelCurrentActions() {
    if(robot == null || !robot.isBusy) {
      panel.SetActive(false);
      return;
    }

    robot.CancelAction();

  }

  public void SetDescription(string desc) {
    text_actionDescription.text = desc;
  }

  public void SetDescription(string verb, ObservedObject selectedObject, ref string description, string period = ".") {
    if(selectedObject == null || CozmoPalette.instance == null) return;
    
    if(description == null || description == string.Empty) description = "Cozmo is attempting to ";
    
    string noun = CozmoPalette.instance.GetNameForObjectType(selectedObject.cubeType);
    string article = "AEIOUaeiou".Contains(noun[0].ToString()) ? "an " : "a ";
    
    description += verb + article + noun + period;

    if(period == ".") SetDescription(description);
  }

  public void SetMute(bool on) {
    mute = on;
  }

}
