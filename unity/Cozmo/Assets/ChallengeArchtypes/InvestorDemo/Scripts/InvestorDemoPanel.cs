using UnityEngine;
using System.Collections;
using Cozmo.UI;

namespace InvestorDemo {
  public class InvestorDemoPanel : BaseModal {
    
    [SerializeField]
    private UnityEngine.UI.Button _NextButton;

    [SerializeField]
    private UnityEngine.UI.Button _MagicButton;

    [SerializeField]
    private UnityEngine.UI.Text _CurrentActionText;

    private void OnEnable() {
      ObjectTagRegistryManager.Instance.RegisterTag("InvestorNextButton", _NextButton.gameObject);
      _MagicButton.onClick.AddListener(HandleMagicButton);
    }

    private void OnDisable() {
      ObjectTagRegistryManager.Instance.RemoveTag("InvestorNextButton");
    }

    private void HandleMagicButton() {
      DAS.Debug(this, "Magic Button!");
      Anki.Cozmo.ExternalInterface.ClearAllObjects clearAllObjectsMessage = new Anki.Cozmo.ExternalInterface.ClearAllObjects();
      clearAllObjectsMessage.robotID = (byte)RobotEngineManager.Instance.CurrentRobotID;
      RobotEngineManager.Instance.Message.ClearAllObjects = clearAllObjectsMessage;
      RobotEngineManager.Instance.SendMessage();
    }

    public void SetActionText(string text) {
      _CurrentActionText.text = text;
    }

    protected override void CleanUp() {

    }
  }

}
