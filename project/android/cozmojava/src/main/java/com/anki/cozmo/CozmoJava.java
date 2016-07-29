package com.anki.cozmo;

import android.app.Activity;

public class CozmoJava {

    public static void init(Activity mainActivity) {
        BackgroundConnectivity.install(mainActivity);
    }

}
