using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;

namespace Anki {
	namespace UI {
		public class AlertView : View
    {

      public static AlertView CreateInstance(string titleKey, string messageKey)
      {
        GameObject gameObj =  (GameObject)Instantiate(Resources.Load("AlertViewDefaultPrefab"));
        AlertView instance = gameObj.GetComponent<AlertView>();

        instance.Title = titleKey;
        instance.Message = messageKey;

        return instance;
      }


      public string Title
      {
        get { return _AlertTitleText.text; }
        set {
          if (_TitleKey != value) {
            _TitleKey = value;
            _AlertTitleText.text = Localization.Get(value).ToUpper();
          }
        }
      }

      public string Message
      {
        get { return _AlertMessageText.text; }
        set { 
          if (_MessageKey != value) {
            _MessageKey = value;
            _AlertMessageText.text = Localization.Get(value);
          } 
        }
      }


      public void SetCloseButtonEnabled(bool enabled)
      {
        _CloseButton.gameObject.SetActive(enabled);
      }

      public void SetCancelButton(string titleKey, Action action = null)
      {
        _SetupButton(_CancelButton, Localization.Get(titleKey), action);
      }

      public void SetButtonOne(string titleKey, Action action = null)
      {
        _SetupButton(_ButtonOne, Localization.Get(titleKey), action);
      }

      public void SetButtonTwo(string titleKey, Action action = null)
      {
        _SetupButton(_ButtonTwo, Localization.Get(titleKey), action);
      }


      // TODO Temp Method to show View. Don't want to pass in gameobject
      public void ShowAlertView(GameObject currentGameObj)
      {
        DAS.Event("ui.alert.show", _TitleKey);
        
        // Find the Root View
        // TODO: Temp Hack
        Transform rootTransfrom = currentGameObj.transform.root;
        gameObject.transform.SetParent(rootTransfrom, false);
        gameObject.transform.SetAsLastSibling();
        // Setup Transform
        RectTransform rectTransform = gameObject.GetComponent<RectTransform>();
        rectTransform.anchorMin = Vector2.zero;
        rectTransform.anchorMax = new Vector2(1.0f, 1.0f);
        rectTransform.offsetMin = Vector2.zero;
        rectTransform.offsetMax = Vector2.zero;
        rectTransform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
      }

      public void DismissAlertView()
      {
        DAS.Event("ui.alert.dismiss", _TitleKey);
        
        RemoveFromSuperview();
        Destroy(gameObject);
      }


      // Lifecycle 


      protected override void OnLoad()
      {
        base.OnLoad();

        _CloseButton.onClick.AddListener(DismissAlertView);
        
        // Hide all buttons
        _CancelButton.gameObject.SetActive(false);
        _CloseButton.gameObject.SetActive(false);
        _ButtonOne.gameObject.SetActive(false);
        _ButtonTwo.gameObject.SetActive(false);
      }

      protected override void OnEnable()
      {
        base.OnEnable();
        //ApplicationServices.OnBackgroundedReturnToHome += OnBackgroundedReturnToHome;
      }

      protected override void OnDisable()
      {
        base.OnDisable();
        //ApplicationServices.OnBackgroundedReturnToHome -= OnBackgroundedReturnToHome;
      }

      protected void OnApplicationPaused() {
        OnBackgroundedReturnToHome();
      }

      private void OnBackgroundedReturnToHome()
      {
        DAS.Info("AlertView.OnBackgrounded", "Killing alert view");
        DismissAlertView();
      }

      // Private 

      [SerializeField]
      private Text _AlertTitleText;
      
      [SerializeField]
      private Text _AlertMessageText;
      
      [SerializeField]
      private AnkiButton _CancelButton;
      
      [SerializeField]
      private AnkiButton _CloseButton;
      
      [SerializeField]
      private AnkiButton _ButtonOne;
      
      [SerializeField]
      private AnkiButton _ButtonTwo;

      private string _TitleKey;
      private string _MessageKey;

      
      
      private void _SetupButton(AnkiButton button, String title, Action action)
      {
        button.gameObject.SetActive(true);
        Text text = button.gameObject.GetComponentInChildren<Text>();
        if (text != null) {
          text.text = title.ToUpper();
        }
        button.onClick.RemoveAllListeners();
        button.onClick.AddListener(() => {
          if (action != null) {
            action();
          }
          DismissAlertView();
        });
      }

		}
	}
}
