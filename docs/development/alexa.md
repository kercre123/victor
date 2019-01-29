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

You can either use the Chewie app to logout, or use the engine console var `ForceAlexaOptOut` under the category `Alexa`, or delete the persistent alexa folder, which is located at `/data/data/com.anki.victor/persistent/alexa` on the robot or at `simulator/controllers/webotsCtrlAnim/persistent/alexa` on mac.


### SDK Details

We use a fork of the avs-device-sdk repo, which is built on our build server as a dependency. At the time of writing, the sdk is compiled as a set of shared libs for both robot and mac. There are plans to make the robot libs static.

[SDK repo](https://github.com/anki/avs-device-sdk)

[SDK documentation](https://alexa.github.io/avs-device-sdk/)

### Troubleshooting

#### Anim process crashes with HTTP error

If you see something like this in the log (along with an 800 on the robot)

```
11-08 12:11:18.000 info logwrapper 2614 2614 vic-anim: /anki/bin/vic-anim: symbol lookup error: /anki/bin/vic-anim: undefined symbol: _ZTVN14alexaClientSDK9avsCommon5utils12libcurlUtils29LibcurlHTTP2ConnectionFactoryE
```

You probably have a stale library in the build directory. Quick fix is to run:

```
find _build/ -name '*.so' -exec rm {} \;
```

to delete all *.so files in _build and then re-build and re-deploy

#### Webots mac build curl HTTP2 error

If you see an error like:

```
```[webotsCtrlAnim] 2018-11-09 04:29:10.149 [  1] E AlexaClientSdkInit:initializeFailed:reason=curlDoesNotSupportHTTP2
[webotsCtrlAnim] 2018-11-09 04:29:10.149 [  1] C ../../../animProcess/src/cozmoAnim/alexa/alexaImpl.cpp:Failed to initialize SDK!
[webotsCtrlAnim] 2018-11-09 04:29:10.149 [  1] E AlexaClientSdkInit:initializeError:reason=notInitialized```
```

You have a version of libcurl without HTTP2 support. Fix is to run:

```
$ brew install curl --with-nghttp2
$ brew link curl --force
```

Then test with:

```
$ curl curl --http2 http://google.com/
```

If you get errors there about not knowing about http2, you may need to `brew uninstall curl` and try again (seems that brew won't over-write it's own curl if the version doesn't update, so if you have the same version but without http2, you need to make it rebuild. There may be a smarter way to do this in brew, but uninstall and reinstall works)

Afterwards, you'll need to rebuild
