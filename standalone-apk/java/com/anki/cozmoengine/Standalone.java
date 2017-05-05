package com.anki.cozmoengine;

import android.content.Context;
import android.content.res.AssetManager;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
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

    private Context mContext;
    private UUID mAppRunId;
    private boolean mRunning;
    private AssetManager mAssetManager;

    private File mPersistentDataPath;
    private File mTemporaryCachePath;
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
        mResourcesBaseFolder = new File(mPersistentDataPath, "cozmo");
        mResourcesFolder = new File(mResourcesBaseFolder, "cozmo_resources");
    }

    public void Begin() {
        if (mRunning) {
            return;
        }
        try {
            mResourcesFolder.mkdirs();
            extractCozmoAssets();
            String cozmoConfig = getCozmoConfig();
            startCozmoEngine(mContext, cozmoConfig);
            mRunning = true;
        } catch (JSONException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void End() {
        if (mRunning) {
            stopCozmoEngine();
            mRunning = false;
        }

    }

    private void extractCozmoAssets() throws IOException{
        InputStream is = mAssetManager.open("resources.txt");
        BufferedReader in = new BufferedReader(new InputStreamReader(is, "UTF-8"));
        String assetPath;
        while ((assetPath = in.readLine()) != null) {
            copyAssetToInternalStorage(assetPath);
        }

    }

    private void copyAssetToInternalStorage(String assetPath) throws IOException {
        File file = new File(mResourcesBaseFolder, assetPath);
        if (assetPath.contains(".")) {
            InputStream is = mAssetManager.open(assetPath);
            OutputStream os = new FileOutputStream(file);
            byte[] buffer = new byte[16384];
            int read;
            while ((read = is.read(buffer)) != -1) {
                os.write(buffer, 0, read);
            }
            is.close();
            os.close();

        } else {
            file.mkdirs();
        }
    }

    private String getCozmoConfig() throws JSONException {

        HashMap<String,String> cozmoConfigMap = new HashMap<String, String>();
        cozmoConfigMap.put("DataPlatformFilesPath", mPersistentDataPath.getAbsolutePath());
        cozmoConfigMap.put("DataPlatformCachePath", mTemporaryCachePath.getAbsolutePath());
        cozmoConfigMap.put("DataPlatformExternalPath", mTemporaryCachePath.getAbsolutePath());
        cozmoConfigMap.put("DataPlatformResourcesPath", mResourcesFolder.getAbsolutePath());
        cozmoConfigMap.put("DataPlatformResourcesBasePath", mResourcesBaseFolder.getAbsolutePath());
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
