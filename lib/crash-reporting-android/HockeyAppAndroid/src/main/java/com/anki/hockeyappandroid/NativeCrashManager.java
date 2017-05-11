// -*- c-basic-offset: 4; -*-
package com.anki.hockeyappandroid;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import net.hockeyapp.android.Constants;
import net.hockeyapp.android.CrashManager;
import net.hockeyapp.android.objects.CrashDetails;
import net.hockeyapp.android.utils.HockeyLog;
import net.hockeyapp.android.utils.HttpURLConnectionBuilder;
import net.hockeyapp.android.utils.SimpleMultipartEntity;

import java.io.BufferedWriter;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.util.Date;

public class NativeCrashManager {
    public static final String DUMPS_DIR = "dmps";
    public static final String DMP_EXTENSION = ".dmp";
    public static final String LOG_EXTENSION = ".faketrace";
    public static final String DESCRIPTION_EXTENSION = ".description";
    private static final long UPDATE_LOG_FREQUENCY_MILLIS = 60 * 1000;

    private static NativeCrashManager sInstance;

    private final NativeCrashManagerListener mListener;
    private final Activity mActivity;
    private final Handler mHandler;
    private final String mDumpsDir;
    private final String mAppRun;
    private final String mUrlString;
    private final String mUser;

    private Runnable mUpdateLogRunnable = new Runnable() {
        @Override
        public void run() {
            updateLogFile();
        }
    };

    public static String getDumpsDirectory(final Activity activity) {
        File dumpsDir = activity.getExternalFilesDir(DUMPS_DIR);
        dumpsDir.mkdir();
        return dumpsDir.getAbsolutePath();
    }

    public static NativeCrashManager getInstance(final Activity activity,
                                                 final String appRun,
                                                 final String identifier,
                                                 final String user,
                                                 final NativeCrashManagerListener listener) {
        if (sInstance == null) {
            sInstance = new NativeCrashManager(activity, appRun, identifier, user, listener);
        }
        return sInstance;
    }

    public static NativeCrashManager getInstance() {
        return sInstance;
    }

    public void updateDescriptionFile(final String description) {
        createDescriptionFile(mAppRun, description);
    }

    public void onDestroy() {
        mHandler.removeCallbacks(mUpdateLogRunnable);
    }

    private NativeCrashManager(final Activity activity,
                               final String appRun,
                               final String identifier,
                               final String user,
                               final NativeCrashManagerListener listener) {
        mListener = listener;
        mActivity = activity;
        mAppRun = appRun;
        mUrlString = "https://rink.hockeyapp.net/api/2/apps/" + identifier + "/crashes/upload";
        mUser = user;
        mHandler = new Handler(Looper.getMainLooper());
        mDumpsDir = getDumpsDirectory(mActivity);
        cleanup();
        updateLogFile();
        uploadDumpFiles();
    }

    private void cleanup() {
        // Cleanup orphaned .faketrace and .description files and empty .dmp files
        String[] orphanedFiles = searchForOrphanedFiles();
        for (final String orphanedFile : orphanedFiles) {
            deleteFile(orphanedFile);
        }
    }

    private boolean deleteFile(final String filename) {
        File file = new File(mDumpsDir, filename);
        return file.delete();
    }

    private boolean moveFile(final String oldPath, final String newPath) {
        boolean result = true;
        File newFile = new File(newPath);
        newFile.delete();
        File oldFile = new File(oldPath);
        try {
            FileInputStream in = new FileInputStream(oldFile);
            FileOutputStream out = new FileOutputStream(newFile);
            byte[] buf = new byte[4096];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
            in.close();
            out.close();
            oldFile.delete();
        } catch (IOException e) {
            Log.d(HockeyLog.HOCKEY_TAG,
                  "Error moving file from '" + oldPath + "' to '" + newPath + "', e = " + e);
            result = false;
        }
        return result;
    }

    private String[] searchForOrphanedFiles() {
        if (mDumpsDir != null) {
            // Try to create the files folder if it doesn't exist
            final File dir = new File(mDumpsDir + File.separator);
            final boolean created = dir.mkdir();
            if (!created && !dir.exists()) {
                return new String[0];
            }

            // Filter for orphaned .faketrace and .description files
            final FilenameFilter filter = new FilenameFilter() {
                @Override
                public boolean accept(final File dir, final String name) {
                    if (!name.endsWith(LOG_EXTENSION)
                        && !name.endsWith(DESCRIPTION_EXTENSION)
                        && !name.endsWith(DMP_EXTENSION)) {
                        // Only consider our file extensions
                        return false;
                    }
                    if (name.startsWith(mAppRun)) {
                        // Don't delete files for the current apprun
                        return false;
                    }
                    // Delete empty .dmp files
                    // Delete .faketrace and .description files whose corresponding
                    // .dmp file doesn't exist or is empty
                    File dmpFile;
                    if (name.endsWith(DMP_EXTENSION)) {
                        dmpFile = new File(dir, name);
                    } else {
                        final String appRun = name.substring(0, name.lastIndexOf("."));
                        dmpFile = new File(dir, appRun + DMP_EXTENSION);
                    }
                    return (!dmpFile.exists() || dmpFile.length() == 0);
                }
            };
            return dir.list(filter);
        } else {
            Log.d(HockeyLog.HOCKEY_TAG, "Can't search for files as file path is null.");
            return new String[0];
        }
    }

    private void updateLogFile() {
        createLogFile(mAppRun);
        // Periodically update the log file so that the Date field is reasonably
        // accurate for when the native crash occurred
        mHandler.postDelayed(mUpdateLogRunnable, UPDATE_LOG_FREQUENCY_MILLIS);
    }

    private String createLogFile(final String appRun) {
        final Date now = new Date();
        long initializeTimestamp = CrashManager.getInitializeTimestamp();
        if (initializeTimestamp == 0) {
            initializeTimestamp = System.currentTimeMillis();
        }
        final Date startDate = new Date(initializeTimestamp);

        CrashDetails crashDetails = new CrashDetails(appRun);
        crashDetails.setIsXamarinException(false);
        crashDetails.setAppPackage(Constants.APP_PACKAGE);
        crashDetails.setAppVersionCode(Constants.APP_VERSION);
        crashDetails.setAppVersionName(Constants.APP_VERSION_NAME);
        crashDetails.setAppStartDate(startDate);
        crashDetails.setAppCrashDate(now);
        crashDetails.setOsVersion(Constants.ANDROID_VERSION);
        crashDetails.setOsBuild(Constants.ANDROID_BUILD);
        crashDetails.setDeviceManufacturer(Constants.PHONE_MANUFACTURER);
        crashDetails.setDeviceModel(Constants.PHONE_MODEL);
        if (Constants.CRASH_IDENTIFIER != null) {
            crashDetails.setReporterKey(Constants.CRASH_IDENTIFIER);
        }
        crashDetails.setThrowableStackTrace("MinidumpContainer");
        crashDetails.writeCrashReport();

        // crashDetails.writeCrashReport() will save the report in a file on the internal
        // storage. We need to move it to the directory where we are storing dumps, which
        // is like on the external storage (aka the sd card).  The moveFile method will copy
        // the file instead of calling File.renameTo, which can fail while trying to cross mount
        // point boundaries.

        final String oldPath = Constants.FILES_PATH + "/" + appRun + ".stacktrace";
        final String filename = appRun + LOG_EXTENSION;
        final String newPath = mDumpsDir + File.separator + filename;
        boolean result = moveFile(oldPath, newPath);
        if (result) {
            return filename;
        }

        return null;
    }

    private String createDescriptionFile(final String appRun, final String description) {
        final String filename = appRun + DESCRIPTION_EXTENSION;
        final String path = mDumpsDir + File.separator + filename;
        try {
            final BufferedWriter write = new BufferedWriter(new FileWriter(path));
            if (description != null) {
                write.write(description);
            }
            write.flush();
            write.close();

            return filename;
        } catch (final Exception e) {
            Log.d(HockeyLog.HOCKEY_TAG, "Error creating description file: " + path + " e = " + e);
        }

        return null;
    }

    private String getLogFileName(final String dumpFilename) {
        final String appRun = dumpFilename.replace(DMP_EXTENSION, "");
        final String filename = dumpFilename.replace(DMP_EXTENSION, LOG_EXTENSION);
        final String path = mDumpsDir + File.separator + filename;
        File logFile = new File(path);
        if (logFile.exists() && logFile.length() > 0) {
            return filename;
        }
        return createLogFile(appRun);
    }

    private String getDescriptionFile(final String dumpFilename) {
        final String appRun = dumpFilename.replace(DMP_EXTENSION, "");
        final String filename = dumpFilename.replace(DMP_EXTENSION, DESCRIPTION_EXTENSION);
        final String path = mDumpsDir + File.separator + filename;
        File descriptionFile = new File(path);
        if (descriptionFile.exists() && descriptionFile.length() > 0) {
            return filename;
        }
        return createDescriptionFile(appRun, null);
    }

    private void uploadDumpFiles() {
        new Thread() {
            @Override
            public void run() {
                final String[] filenames = searchForDumpFiles();
                for (final String dumpFilename : filenames) {
                    final String logFilename = getLogFileName(dumpFilename);
                    final String descriptionFilename = getDescriptionFile(dumpFilename);
                    if (logFilename != null) {
                        uploadDumpAndLog(dumpFilename, logFilename, descriptionFilename);
                    }
                }
            }
        }.start();
    }

    private void uploadDumpAndLog(final String dumpFilename, final String logFilename,
                                  final String descriptionFilename) {
        try {
            if (mListener != null) {
                final String appRunID = dumpFilename.split("\\.(?=[^\\.]+$)")[0];
                mListener.onUploadCrash(appRunID);
            }

            SimpleMultipartEntity entity = new SimpleMultipartEntity();
            entity.writeFirstBoundaryIfNeeds();

            entity.addPart("userID", mUser);
            final File dumpFile = new File(mDumpsDir, dumpFilename);
            entity.addPart("attachment0", dumpFile, false);
            final File logFile = new File(mDumpsDir, logFilename);

            final File descriptionFile = new File(mDumpsDir, descriptionFilename);
            boolean haveDescriptionFile =
                descriptionFile.exists() && descriptionFile.length() > 0;
            entity.addPart("log", logFile, !haveDescriptionFile);
            if (haveDescriptionFile) {
                entity.addPart("description", descriptionFile, true);
            }
            entity.writeLastBoundaryIfNeeds();

            HttpURLConnection urlConnection = new HttpURLConnectionBuilder(mUrlString)
                .setRequestMethod("POST")
                .setHeader("Content-Type", entity.getContentType())
                .build();
            urlConnection.setRequestProperty("Content-Length",
                                             String.valueOf(entity.getContentLength()));

            BufferedOutputStream outputStream =
                new BufferedOutputStream(urlConnection.getOutputStream());

            outputStream.write(entity.getOutputStream().toByteArray());
            outputStream.flush();
            outputStream.close();

            int statusCode = urlConnection.getResponseCode();
            boolean successful = (statusCode == HttpURLConnection.HTTP_ACCEPTED
                                  || statusCode == HttpURLConnection.HTTP_CREATED);
            if (successful) {
                Log.v(HockeyLog.HOCKEY_TAG,
                      "Successfully uploaded dump file: " + dumpFilename
                      + ", statusCode = " + statusCode);
                deleteFile(logFilename);
                deleteFile(dumpFilename);
                deleteFile(descriptionFilename);
            } else {
                Log.d(HockeyLog.HOCKEY_TAG,
                      "Error uploading dump file: " + dumpFilename
                      + ", statusCode = " + statusCode);
            }
        } catch (final Exception e) {
            Log.d(HockeyLog.HOCKEY_TAG,
                  "Error uploading dump file: " + dumpFilename + ", e = " + e);
            e.printStackTrace();
        }
    }

    private String[] searchForDumpFiles() {
        if (mDumpsDir != null) {
            // Try to create the files folder if it doesn't exist
            final File dir = new File(mDumpsDir + File.separator);
            final boolean created = dir.mkdir();
            if (!created && !dir.exists()) {
                return new String[0];
            }

            // Filter for DMP_EXTENSION files
            final FilenameFilter filter = new FilenameFilter() {
                @Override
                public boolean accept(final File dir, final String name) {
                    return name.endsWith(DMP_EXTENSION) && !name.startsWith(mAppRun);
                }
            };
            return dir.list(filter);
        } else {
            Log.d(HockeyLog.HOCKEY_TAG, "Can't search for dump files as file path is null.");
            return new String[0];
        }
    }
}
