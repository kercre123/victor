# Alexa usage

### Creating a developer account

You will need to link your robot and/or mac laptop with an Amazon developer account. If you don't already have one, you may create a new one at [developer.amazon.com](https://developer.amazon.com). Note that their account system supports "+" email suffixes like developer+3@anki.com. Obviously, select "NO" for "Do you plan to monetize."

### Enabling Alexa
Alexa is disabled by default. To test or develop with Alexa, first ensure that the feature flag `Alexa` is enabled in [features.json](/resources/config/features.json). Then, either use the Chewie app to opt in, or use the engine console var `ForceAlexaOptIn` under the category `Alexa`. 

You will need to obtain a code and URL from the robot in order to authenticate it with Amazon. The design calls for the robot to show the code and URL. If that hasn't yet been implemented, then both code and URL should appear in `victor_log | grep Alexa` during the authentication process. The URL is usually [amazon.com/us/code](https://amazon.com/us/code).

Once authenticated, the robot (or webots) will have Alexa enabled by default on subsequent runs. See [Disabling Alexa](#disabling) for details on logging out.

Opting in to Alexa also enables the second wake-word, "Alexa." To get started, try saying "Alexa, tell me a joke." She has quite the sense of humor!

### Setting preferences

We don't (and can't!) sync robot settings with alexa settings, so things like timezone, location, and locale are all set to Alexa defaults. To change these, visit [alexa.amazon.com](https://alexa.amazon.com).

### Disabling Alexa  <a name="disabling"></a>

This is still in design at the time of writing. 

You can either use the Chewie app to logout, or use the engine console var `ForceAlexaOptOut` under the category `Alexa`, or delete the persistent alexa folder, which is located at `/data/data/com.anki.victor/alexa` on the robot or at `simulator/controllers/webotsCtrlAnim/persistent/alexa` on mac.


### SDK Details

We use a fork of the avs-device-sdk repo, which is built as part of coretech-external. At the time of writing, the sdk is compiled as a set of shared libs for both robot and mac. There are plans to make the robot libs static. Also, the sdk has a number of dependencies that are also compiled in coretech-external.

[SDK repo](https://github.com/anki/avs-device-sdk)

[SDK documentation](https://alexa.github.io/avs-device-sdk/)