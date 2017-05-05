// -*- c-basic-offset: 4; -*-
package com.anki.cozmoengine;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;
import android.widget.TextView;

public class MainActivity extends Activity {

    private final static String TAG = "CozmoEngine2";

    private Standalone mStandalone;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "onCreate");
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        mStandalone = new Standalone(this);

        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText("Standalone CozmoEngine");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "onResume");
        mStandalone.Begin();
    }

    @Override
    protected void onDestroy() {
        Log.i(TAG, "onDestroy");
        mStandalone.End();
        super.onDestroy();
    }
}
