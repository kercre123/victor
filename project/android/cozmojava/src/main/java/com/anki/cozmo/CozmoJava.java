package com.anki.cozmo;

import android.app.Activity;

import com.anki.util.AnkitivityDispatcher;

public class CozmoJava {

    public static void init(Activity mainActivity, AnkitivityDispatcher dispatcher) {
        BackgroundConnectivity.install(mainActivity);
        CozmoWifi.register(mainActivity, dispatcher);
    }

}
