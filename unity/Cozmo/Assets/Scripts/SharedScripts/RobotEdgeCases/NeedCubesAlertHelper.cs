using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.UI {
  public class NeedCubesAlertHelper : MonoBehaviour {
    public static void OpenNeedCubesAlert(int currentCubes, int neededCubes, string titleString,
                                   ModalPriorityData basePriorityData) {
      var needCubesPriorityData = ModalPriorityData.CreateSlightlyHigherData(basePriorityData);
      AlertModalButtonData openCubeHelpButtonData = CreateCubeHelpButtonData(needCubesPriorityData);
      AlertModalData needCubesData = CreateNeedMoreCubesAlertData(openCubeHelpButtonData, currentCubes,
                      neededCubes, titleString);

      UIManager.OpenAlert(needCubesData, needCubesPriorityData);
    }

    private static AlertModalButtonData CreateCubeHelpButtonData(ModalPriorityData basePriorityData) {
      var cubeHelpModalPriorityData = ModalPriorityData.CreateSlightlyHigherData(basePriorityData);
      System.Action cubeHelpButtonPressed = () => {
        UIManager.OpenModal(AlertModalLoader.Instance.CubeHelpModalPrefab, cubeHelpModalPriorityData, null);
      };
      return new AlertModalButtonData("open_cube_help_modal_button",
              LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalButton,
              cubeHelpButtonPressed);
    }

    private static AlertModalData CreateNeedMoreCubesAlertData(AlertModalButtonData primaryButtonData,
                              int currentCubes, int neededCubes, string titleString) {
      int differenceCubes = neededCubes - currentCubes;
      object[] descLocArgs = new object[] {
    differenceCubes,
    (ItemDataConfig.GetCubeData().GetAmountName(differenceCubes)),
    titleString
    };

      return new AlertModalData("game_needs_more_cubes_alert",
              LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalTitle,
              LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalDescription,
              primaryButtonData, showCloseButton: true, descLocArgs: descLocArgs);
    }
  }
}