using Anki.Cozmo;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.UI {
  public class LightCubeSprite : MonoBehaviour {
    [SerializeField]
    private Image[] _LightSpritesClockwise;

    [SerializeField]
    private Image _CubeIcon;

    [SerializeField]
    private CanvasGroup _AlphaController;

    [SerializeField]
    private Sprite _LightCubeOneSprite;

    [SerializeField]
    private Sprite _LightCubeTwoSprite;

    [SerializeField]
    private Sprite _LightCubeThreeSprite;

    private LightCube _CurrentLightCube;

    private void OnDestroy() {
      if (_CurrentLightCube != null) {
        _CurrentLightCube.OnLightsChanged -= HandleLightCubeStateChanged;
      }
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.CubeLightsStateTransition>(HandleLightCubeStateChanged);
    }

    public void SetIcon(ObjectType objectType) {
      switch (objectType) {
      case ObjectType.Block_LIGHTCUBE1:
        _CubeIcon.sprite = _LightCubeOneSprite;
        break;
      case ObjectType.Block_LIGHTCUBE2:
        _CubeIcon.sprite = _LightCubeTwoSprite;
        break;
      case ObjectType.Block_LIGHTCUBE3:
        _CubeIcon.sprite = _LightCubeThreeSprite;
        break;
      default:
        _CubeIcon.gameObject.SetActive(false);
        break;
      }
    }

    public void SetAlpha(float alpha) {
      if (_AlphaController != null) {
        _AlphaController.alpha = alpha;
      }
    }

    public void SetColor(Color color) {
      foreach (Image image in _LightSpritesClockwise) {
        SetLightSpriteColor(image, color);
      }
    }

    public void SetColors(Color[] colors) {
      Color targetColor;
      for (int i = 0; i < _LightSpritesClockwise.Length; i++) {
        targetColor = colors[i % colors.Length];
        SetLightSpriteColor(_LightSpritesClockwise[i], targetColor);
      }
    }

    public void FollowLightCube(LightCube lightCube, bool isFreeplay) {
      if (lightCube != null) {
        // If the current light cube is not null, stop listening to it
        if (_CurrentLightCube != null) {
          StopFollowLightCube();
        }

        // Listen to light cube for light updates
        _CurrentLightCube = lightCube;

        if (isFreeplay) {
          RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.CubeLightsStateTransition>(HandleLightCubeStateChanged);
        }
        else {
          _CurrentLightCube.OnLightsChanged += HandleLightCubeStateChanged;
        }
      }
    }

    public void StopFollowLightCube() {
      // Stop listening to LightCube to update lights
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.CubeLightsStateTransition>(HandleLightCubeStateChanged);
      SetColor(Color.black);

      if (_CurrentLightCube != null) {
        _CurrentLightCube.OnLightsChanged -= HandleLightCubeStateChanged;
      }
      _CurrentLightCube = null;
    }

    private void HandleLightCubeStateChanged() {
      if (_CurrentLightCube != null) {
        SetColorsBasedOnLights(_CurrentLightCube.Lights);
      }
    }

    private void SetColorsBasedOnLights(ActiveObject.Light[] lights) {
      ActiveObject.Light targetLight;
      for (int i = 0; i < _LightSpritesClockwise.Length; i++) {
        targetLight = lights[i % lights.Length];
        Color onColor = targetLight.OnColor.ToColor();
        Color offColor = targetLight.OffColor.ToColor();

        Color colorToShow = (onColor == Color.black && offColor != Color.black) ? offColor : onColor;
        SetLightSpriteColor(_LightSpritesClockwise[i], colorToShow);

        // TODO - Polish: If requested, handle pulsing/rotation here, since engine doesn't send us
        // pulsing frame by frame we have to use the light data to pulse it ourselves.
      }
    }

    private void HandleLightCubeStateChanged(Anki.Cozmo.ExternalInterface.CubeLightsStateTransition message) {
      if (_CurrentLightCube != null && message.factoryID == _CurrentLightCube.FactoryID) {
        // Update light colors of sprite with new light cube colors
        SetColorsBasedOnLights(message.lights);
      }
    }

    private void SetColorsBasedOnLights(LightState[] lights) {
      LightState targetLight;
      for (int i = 0; i < _LightSpritesClockwise.Length; i++) {
        targetLight = lights[i % lights.Length];
        Color onColor = targetLight.onColor.ToColor();
        Color offColor = targetLight.offColor.ToColor();

        Color colorToShow = (onColor == Color.black && offColor != Color.black) ? offColor : onColor;

        // ushorts are from engine; Show colors at full brightness even if lights are half bright IRL
        colorToShow *= 2;

        SetLightSpriteColor(_LightSpritesClockwise[i], colorToShow);

        // TODO - Polish: If requested, handle pulsing/rotation here, since engine doesn't send us
        // pulsing frame by frame we have to use the light data to pulse it ourselves.
      }
    }

    private void SetLightSpriteColor(Image image, Color color) {
      if (image != null) {
        image.gameObject.SetActive(color != Color.black);
        image.color = color;
      }
    }
  }
}