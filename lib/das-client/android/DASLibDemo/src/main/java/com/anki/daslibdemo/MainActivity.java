/**
 * File: MainActivity.java
 *
 * Author: seichert
 * Created: 07/08/2014
 *
 * Description: Java Activity that calls functions in DAS
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

package com.anki.daslibdemo;

import com.anki.daslib.DAS;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends Activity {

    private static final String DAS_CONFIG_FILE_NAME = "DASConfig.json";
    private File gameLogDir;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        gameLogDir = new File(getExternalCacheDir(), "GameLogs");
        gameLogDir.mkdirs();

        Log.v("daslibdemo", "onCreate(final Bundle savedInstanceState)");

        final TextView tv = new TextView(this);
        tv.setText("Calling DAS functions");
        setContentView(tv);


        testDisableEnableDAS();
        testLoggingToDAS();
        testBackgroundThreadGlobals();
        // testPostToServer();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.v("daslibdemo", "onDestroy()");
        DAS.Close();
    }

    @Override
    public void onPause() {
        super.onPause();
        Log.v("daslibdemo", "onPause()");
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.v("daslibdemo", "onResume()");
    }

    @Override
    public void onStart() {
        super.onStart();
        Log.v("daslibdemo", "onStart()");
    }

    @Override
    public void onStop() {
        super.onStop();
        Log.v("daslibdemo", "onStop()");
    }

    private void assertEqual(int expected, int actual) {
        assertEqual("", expected, actual);
    }

    private void assertEqual(String message, int expected, int actual) {
        if (actual != expected) {
            Log.e("daslibdemo", String.format("Expected %d. Actual %d. %s", expected, actual, message));
        }
    }

    private void assertNotZero(String message, int actual) {
        if (0 == actual) {
            Log.e("daslibdemo", String.format("Expected not zero. %s", message));
        }
    }

    private void testDisableEnableDAS() {

        if (!BuildConfig.DEBUG) {
        DAS.DisableNetwork(DAS.DisableNetworkReason_Simulator);
        assertEqual(DAS.DisableNetworkReason_Simulator, DAS.GetNetworkingDisabled());

        DAS.EnableNetwork(DAS.DisableNetworkReason_Simulator);
        assertEqual(0, DAS.GetNetworkingDisabled());

        DAS.DisableNetwork(DAS.DisableNetworkReason_UserOptOut);
        assertEqual(DAS.DisableNetworkReason_UserOptOut, DAS.GetNetworkingDisabled());
        DAS.EnableNetwork(DAS.DisableNetworkReason_UserOptOut);
        assertEqual(0, DAS.GetNetworkingDisabled());

        DAS.DisableNetwork(DAS.DisableNetworkReason_Simulator);
        DAS.DisableNetwork(DAS.DisableNetworkReason_UserOptOut);
        assertEqual(DAS.DisableNetworkReason_Simulator | DAS.DisableNetworkReason_UserOptOut,
                    DAS.GetNetworkingDisabled());
        DAS.EnableNetwork(DAS.DisableNetworkReason_Simulator);
        DAS.EnableNetwork(DAS.DisableNetworkReason_UserOptOut);
        assertEqual(0, DAS.GetNetworkingDisabled());
        } else {
            assertNotZero("In DEBUG, DASNetworkingDisabled should always be non-zero", DAS.GetNetworkingDisabled());
        }
    }

    private void testLoggingToDAS() {
        configureDAS();

        DAS.SetGlobal("$duser", "daslibdemo");
        DAS.SetGlobal("$app", "1.1");

        DAS.Event("ApplicationLaunched", "DASLibDemo");

        DAS.Debug("AppStart", "debug %d, %s, %f", 1, "2", 3.0);
        DAS.Info("AppStart",  "info  %d, %s, %f", 1, "2", 3.0);
        // Log an event with control characters, double quotes, etc. to make sure it is escaped correctly
        DAS.Event("AppStart", "%s", "\"\'`\\/\b\f\n\r\t\u001b\"");
        // Log an event with some non-ASCII unicode characters
        DAS.Event("AppStart", "%s", "\u20acs and \u00a2s");
        DAS.Warn("AppStart",  "warn  %d, %s, %f", 1, "2", 3.0);
        DAS.Error("AppStart", "error %d, %s, %f", 1, "2", 3.0);

        dasEventOnBackgroundThread("AppStart", "background thread 1 - evt");
        dasEventOnBackgroundThread("AppStart", "background thread 2 - evt");

        testSetLevel();
        // testFloodingTheLog();
    }

    private void SetGameGlobal(String gameId) {
        if (gameId != null) {
            File gameIdDir = new File(gameLogDir, gameId);
            gameIdDir.mkdirs();
        }
        DAS.SetGlobal("$game", gameId);
    }

    private void testBackgroundThreadGlobals() {
        new Thread(new Runnable() {
                public void run() {
                    DAS.SetGlobal("$apprun", "background-thread-1");
                    DAS.SetGlobal("$phys", "39393-AABBDD");
                    DAS.Event("AppStart", "on the background thread - 1");
                    DAS.Event("AppStart", "on the background thread - 2");
                    DAS.SetGlobal("$unit", "unit-id-background-thread");
                    SetGameGlobal("game-id-1");
                    DAS.Event("AppStart", "on the background thread - game id is set");
                    SetGameGlobal(null);
                    DAS.Event("AppStart", "on the background thread - game id is cleared");

                    // Create another thread to verify that $apprun, $phys, and $unit, but
                    // not $game are set for the other thread
                    new Thread(new Runnable() {
                            public void run() {
                                DAS.Event("AppStart", "on the back-background thread - 1");
                                DAS.Event("AppStart", "on the back-background thread - 2");
                            }
                        }).start();
                }
            }).start();
    }

    private void configureDAS() {
        copyAssetToInternalStorage(getAssets(), DAS_CONFIG_FILE_NAME);

        File dasConfigFile = new File(getFilesDir(), DAS_CONFIG_FILE_NAME);
        File dasConfigOverrideFile = new File(getExternalFilesDir(null), DAS_CONFIG_FILE_NAME);

        if (dasConfigOverrideFile.exists()) {
            dasConfigFile = dasConfigOverrideFile;
            Log.v("daslibdemo", "Using override configuration file at " + dasConfigFile.getAbsolutePath());
        }

        DAS.Configure(dasConfigFile.getAbsolutePath(),
                      new File(getExternalCacheDir(), "DASLogs").getAbsolutePath(),
                      gameLogDir.getAbsolutePath());
    }

    private void copyAssetToInternalStorage(AssetManager am, String fileName) {
        File file = new File(getFilesDir(), fileName);
        try {
            InputStream is = am.open(fileName);
            OutputStream os = new FileOutputStream(file);
            byte[] data = new byte[is.available()];
            is.read(data);
            os.write(data);
            is.close();
            os.close();
        } catch (IOException e) {
            Log.w("InternalStorage", "Error writing " + file, e);
        }
    }

    private void dasEventOnBackgroundThread(final String event, final String eventValue) {
        new Thread(new Runnable() {
                public void run() {
                    DAS.Event(event, eventValue);
                }
            }).start();
    }

    private void testFloodingTheLog() {
        new Thread(new Runnable() {
                public void run() {
                    // Do a whole bunch of DAS writes to flood the log and make it roll over
                    for (int i = 1 ; i < 40000 ; ++i) {
                        DAS.Event("FloodTheLog", "iteration i = %d", i);
                    }
                }
            }).start();
    }

    private void testSetLevel() {
        DAS.SetLevel("DoNotLogMe", DAS.LogLevel_Debug);
        DAS.Event("DoNotLogMe", "%s", "dummy value");
    }

    private void testPostToServer() {
        new Thread(new Runnable() {
                public void run() {
                    // If testing the DAS.postToServer routine, replace the URL below with your own.
                    DAS.postToServer("http://10.77.0.151/~seichert/store.cgi", "{\"key\":\"value\"}");
                }
            }).start();
    }
}
