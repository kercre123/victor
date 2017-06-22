package com.anki.cozmoengine;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.UUID;

/**
 * Created by seichert on 11/25/16.
 */

public class Standalone {
    protected static final String TAG = "CozmoEngine2";//OverDriveActivity.class.getSimpleName();

    private Context mContext;
    private UUID mAppRunId;
    private boolean mRunning;
    private AssetManager mAssetManager;

    private File mPersistentDataPath;
    private File mTemporaryCachePath;
    private File mExternalAssetsPath;
    private File mResourcesBaseFolder;
    private File mResourcesFolder;

    public native void startCozmoEngine(Context context, String configuration);
    public native void stopCozmoEngine();

    public Standalone(final Context context) {
        mContext = context;
        mAppRunId = UUID.randomUUID();
        mAssetManager = context.getAssets();
        mRunning = false;

        mPersistentDataPath = context.getFilesDir();
        mTemporaryCachePath = context.getCacheDir();
       
        File externalFilesPath = context.getExternalFilesDir(null);
        mExternalAssetsPath = new File(externalFilesPath, "assets");

        Log.i(TAG, "ExternalAssetsPath: " + mExternalAssetsPath.getAbsolutePath());
    }

    public void Begin() {
        if (mRunning) {
            return;
        }
        try {
            mExternalAssetsPath.mkdirs();
            String cozmoConfig = getCozmoConfig();
            startCozmoEngine(mContext, cozmoConfig);
            mRunning = true;
        } catch (JSONException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void End() {
        if (mRunning) {
            stopCozmoEngine();
            mRunning = false;
        }

    }

    private String getCozmoAssetsRef() throws IOException {
        InputStream is = mAssetManager.open("cozmo_assets.ref");
        BufferedReader in = new BufferedReader(new InputStreamReader(is, "UTF-8"));
        String assetsHash = in.readLine();
        return assetsHash;
    }

    private File getCozmoAssetsDir() throws IOException {
        String cozmoAssetsRef = null;
        try {
            cozmoAssetsRef = getCozmoAssetsRef();
        } catch (IOException e) {
            Log.e(TAG, "getCozmoConfig :: missing cozmo_assets.ref file");
            throw(e);
        }

        File assetsDir = new File(mExternalAssetsPath, cozmoAssetsRef);

        return assetsDir;
    }

    private String getCozmoConfig() throws JSONException, IOException {
        File cozmoAssetsDir = getCozmoAssetsDir();
        if (!cozmoAssetsDir.isDirectory()) {
            String ref = getCozmoAssetsRef();
            Log.e(TAG, "getCozmoConfig :: assets not found in: " + cozmoAssetsDir);
            Log.e(TAG, "getCozmoConfig :: re-deploy assets for hash/ref: " + ref);
        }

        File cozmoResources = new File(cozmoAssetsDir, "cozmo_resources");
        Log.i(TAG, "cozmoAssetsDir: " + cozmoAssetsDir);

        HashMap<String,String> cozmoConfigMap = new HashMap<String, String>();
        cozmoConfigMap.put("DataPlatformFilesPath", mPersistentDataPath.getAbsolutePath());
        cozmoConfigMap.put("DataPlatformCachePath", mTemporaryCachePath.getAbsolutePath());
        cozmoConfigMap.put("DataPlatformExternalPath", mTemporaryCachePath.getAbsolutePath());
        cozmoConfigMap.put("DataPlatformResourcesPath", cozmoResources.getAbsolutePath());
        cozmoConfigMap.put("DataPlatformResourcesBasePath", cozmoAssetsDir.getAbsolutePath());
        cozmoConfigMap.put("appRunId", mAppRunId.toString());

        JSONObject cozmoConfigJsonObject = new JSONObject(cozmoConfigMap);
        cozmoConfigJsonObject = cozmoConfigJsonObject.put("standalone", true);

        return cozmoConfigJsonObject.toString(2);
    }

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("DAS");
        System.loadLibrary("cozmoEngine2");
    }


}
