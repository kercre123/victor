using Cozmo.UI;

namespace Onboarding {

  public class OnboardingNotificationStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();
      // bring up notification preprompt.
      if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.OSNotificationsPermissionsPromptShown) {
        // Since we don't need any permissions skip this stage
        OnboardingManager.Instance.GoToNextStage();
      }
      else {
        // Pop up window telling you we will ask for notifications
        AlertModalData modalData = new AlertModalData("notifications_alert",
                        LocalizationKeys.kSettingsNotificationpanelTitle,
                        LocalizationKeys.kOnboardingNotificationprepromptBody,
                        new AlertModalButtonData("yes_notifications_button", LocalizationKeys.kButtonYes, HandleShowOSNotificationsPrompt),
                        new AlertModalButtonData("no_notifications_button", LocalizationKeys.kButtonMaybeLater, HandleMaybeLaterClicked));

        var modalPriority = new ModalPriorityData(ModalPriorityLayer.VeryLow, 2,
                    LowPriorityModalAction.CancelSelf,
                    HighPriorityModalAction.Stack);

        UIManager.OpenAlert(modalData, modalPriority, overrideCloseOnTouchOutside: false);
      }
    }

    private void HandleShowOSNotificationsPrompt() {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.OSNotificationsPermissionsPromptShown = true;
      Cozmo.Notifications.NotificationsManager.Instance.InitNotificationsIfAllowed();
      OnboardingManager.Instance.GoToNextStage();
    }

    private void HandleMaybeLaterClicked() {
      OnboardingManager.Instance.GoToNextStage();
      // "later" is a week from now.
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.LastTimeAskedAboutNotifications = System.DateTime.Now;
    }

    public override void OnDestroy() {
      base.OnDestroy();
    }

  }

}
