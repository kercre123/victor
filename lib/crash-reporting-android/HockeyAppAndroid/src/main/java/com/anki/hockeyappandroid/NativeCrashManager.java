// -*- c-basic-offset: 4; -*-
package com.anki.hockeyappandroid;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import net.hockeyapp.android.Constants;

import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.mime.MultipartEntity;
import org.apache.http.entity.mime.content.FileBody;
import org.apache.http.entity.mime.content.StringBody;
import org.apache.http.HttpStatus;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.HttpResponse;
import org.apache.http.StatusLine;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.FilenameFilter;
import java.util.Date;
import java.util.UUID;

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
    private final String mIdentifier;
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
        mIdentifier = identifier;
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
            Log.d("HockeyApp", "Can't search for files as file path is null.");
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
        final String filename = appRun + LOG_EXTENSION;
        final String path = mDumpsDir + File.separator + filename;

        try {
            // Write the faketrace to disk
            final BufferedWriter write = new BufferedWriter(new FileWriter(path));
            write.write("Package: " + Constants.APP_PACKAGE + "\n");
            write.write("Version: " + Constants.APP_VERSION + "\n");
            write.write("Android: " + Constants.ANDROID_VERSION + "\n");
            write.write("Manufacturer: " + Constants.PHONE_MANUFACTURER + "\n");
            write.write("Model: " + Constants.PHONE_MODEL + "\n");
            write.write("Date: " + now + "\n");
            write.write("\n");
            write.write("MinidumpContainer");
            write.flush();
            write.close();

            return filename;
        } catch (final Exception e) {
            Log.d("HockeyApp", "Error creating trace file: " + path + " e = " + e);
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
            Log.d("HockeyApp", "Error creating description file: " + path + " e = " + e);
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
        final String[] filenames = searchForDumpFiles();
        for (final String dumpFilename : filenames) {
            final String logFilename = getLogFileName(dumpFilename);
            final String descriptionFilename = getDescriptionFile(dumpFilename);
            if (logFilename != null) {
                uploadDumpAndLog(dumpFilename, logFilename, descriptionFilename);
            }
        }
    }

    private void uploadDumpAndLog(final String dumpFilename, final String logFilename,
                                  final String descriptionFilename) {
        new Thread() {
            @Override
            public void run() {
                try {
                    final DefaultHttpClient httpClient = new DefaultHttpClient();
                    final HttpPost httpPost =
                        new HttpPost("https://rink.hockeyapp.net/api/2/apps/" + mIdentifier
                                     + "/crashes/upload");

                    final MultipartEntity entity = new MultipartEntity();

                    if (mListener != null) {
                        final String appRunID = dumpFilename.split("\\.(?=[^\\.]+$)")[0];
                        mListener.onUploadCrash(appRunID);
                    }

                    final File dumpFile = new File(mDumpsDir, dumpFilename);
                    entity.addPart("attachment0", new FileBody(dumpFile));

                    final File logFile = new File(mDumpsDir, logFilename);
                    entity.addPart("log", new FileBody(logFile));

                    final File descriptionFile = new File(mDumpsDir, descriptionFilename);
                    if (descriptionFile.exists() && descriptionFile.length() > 0) {
                        entity.addPart("description", new FileBody(descriptionFile));
                    }

                    entity.addPart("userID", new StringBody(mUser));

                    httpPost.setEntity(entity);

                    final HttpResponse httpResponse = httpClient.execute(httpPost);
                    final StatusLine statusLine = httpResponse.getStatusLine();
                    if (statusLine != null) {
                        int statusCode = statusLine.getStatusCode();
                        if (statusCode >= HttpStatus.SC_OK
                            && statusCode < HttpStatus.SC_MULTIPLE_CHOICES) {
                            Log.v("HockeyApp",
                                  "Successfully uploaded dump file: " + dumpFilename
                                  + ", statusCode = " + statusCode);
                            deleteFile(logFilename);
                            deleteFile(dumpFilename);
                            deleteFile(descriptionFilename);
                        } else {
                            Log.d("HockeyApp",
                                  "Error uploading dump file: " + dumpFilename
                                  + ", statusCode = " + statusCode);
                        }
                    } else {
                        Log.d("HockeyApp",
                              "Error uploading dump file: " + dumpFilename
                              + ", no statusLine in response");
                    }
                } catch (final Exception e) {
                    Log.d("HockeyApp", "Error uploading dump file: " + dumpFilename + ", e = " + e);
                    e.printStackTrace();
                }
            }
        }.start();
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
            Log.d("HockeyApp", "Can't search for dump files as file path is null.");
            return new String[0];
        }
    }
}
